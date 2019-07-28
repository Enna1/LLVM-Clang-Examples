#include "bf_jit.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <memory>
#include <string>

constexpr int MEMORY_SIZE = 30000;
const char* const JIT_FUNC_NAME = "__llvmjit";

using BracketBlocks = std::pair<llvm::BasicBlock*, llvm::BasicBlock*>;

struct BFProgram
{
    std::string instructions;
};

BFProgram parse_from_stream(std::istream& stream)
{
    BFProgram program;

    for (std::string line; std::getline(stream, line);)
    {
        for (auto c : line)
        {
            if (c == '>' || c == '<' || c == '+' || c == '-' || c == '.' || c == ',' ||
                c == '[' || c == ']')
            {
                program.instructions.push_back(c);
            }
        }
    }
    return program;
}

llvm::Function* emit_jit_function(const BFProgram& program, llvm::Module* module,
                                  llvm::Function* putchar_fn, llvm::Function* getchar_fn)
{
    llvm::LLVMContext& context = module->getContext();

    llvm::Type* int32_type = llvm::Type::getInt32Ty(context);
    llvm::Type* int8_type = llvm::Type::getInt8Ty(context);
    llvm::Type* void_type = llvm::Type::getVoidTy(context);

    llvm::FunctionType* jit_fn_type = llvm::FunctionType::get(void_type, {}, false);
    llvm::Function* jit_fn = llvm::Function::Create(
        jit_fn_type, llvm::Function::ExternalLinkage, JIT_FUNC_NAME, module);

    llvm::BasicBlock* entry_bb = llvm::BasicBlock::Create(context, "entry", jit_fn);
    llvm::IRBuilder<> builder(entry_bb);

    // Create stack allocations for the memory and the data pointer. The memory
    // is memset to zeros. The data pointer is used as an offset into the memory
    // array; it is initialized to 0.
    llvm::AllocaInst* memory =
        builder.CreateAlloca(int8_type, builder.getInt32(MEMORY_SIZE), "memory");
    builder.CreateMemSet(memory, builder.getInt8(0), MEMORY_SIZE, 1);
    llvm::AllocaInst* dataptr_addr =
        builder.CreateAlloca(int32_type, nullptr, "dataptr_addr");
    builder.CreateStore(builder.getInt32(0), dataptr_addr);

    std::stack<BracketBlocks> open_bracket_stack;

    for (size_t pc = 0; pc < program.instructions.size(); ++pc)
    {
        char instruction = program.instructions[pc];
        switch (instruction)
        {
        case '>':
        {
            llvm::Value* dataptr = builder.CreateLoad(dataptr_addr, "dataptr");
            llvm::Value* inc_dataptr =
                builder.CreateAdd(dataptr, builder.getInt32(1), "inc_dataptr");
            builder.CreateStore(inc_dataptr, dataptr_addr);
            break;
        }
        case '<':
        {
            llvm::Value* dataptr = builder.CreateLoad(dataptr_addr, "dataptr");
            llvm::Value* dec_dataptr =
                builder.CreateSub(dataptr, builder.getInt32(1), "dec_dataptr");
            builder.CreateStore(dec_dataptr, dataptr_addr);
            break;
        }
        case '+':
        {
            llvm::Value* dataptr = builder.CreateLoad(dataptr_addr, "dataptr");
            llvm::Value* element_addr =
                builder.CreateInBoundsGEP(memory, { dataptr }, "element_addr");
            llvm::Value* element = builder.CreateLoad(element_addr, "element");
            llvm::Value* inc_element =
                builder.CreateAdd(element, builder.getInt8(1), "inc_element");
            builder.CreateStore(inc_element, element_addr);
            break;
        }
        case '-':
        {
            llvm::Value* dataptr = builder.CreateLoad(dataptr_addr, "dataptr");
            llvm::Value* element_addr =
                builder.CreateInBoundsGEP(memory, { dataptr }, "element_addr");
            llvm::Value* element = builder.CreateLoad(element_addr, "element");
            llvm::Value* dec_element =
                builder.CreateSub(element, builder.getInt8(1), "sub_element");
            builder.CreateStore(dec_element, element_addr);
            break;
        }
        case '.':
        {
            llvm::Value* dataptr = builder.CreateLoad(dataptr_addr, "dataptr");
            llvm::Value* element_addr =
                builder.CreateInBoundsGEP(memory, { dataptr }, "element_addr");
            llvm::Value* element = builder.CreateLoad(element_addr, "element");
            llvm::Value* element_i32 =
                builder.CreateIntCast(element, int32_type, false, "element_i32");
            builder.CreateCall(putchar_fn, element_i32);
            break;
        }
        case ',':
        {
            llvm::Value* user_input = builder.CreateCall(getchar_fn, {}, "user_input");
            llvm::Value* user_input_i8 =
                builder.CreateIntCast(user_input, int8_type, false, "user_input_i8");
            llvm::Value* dataptr = builder.CreateLoad(dataptr_addr, "dataptr");
            llvm::Value* element_addr =
                builder.CreateInBoundsGEP(memory, { dataptr }, "element_addr");
            builder.CreateStore(user_input_i8, element_addr);
            break;
        }
        case '[':
        {
            llvm::Value* dataptr = builder.CreateLoad(dataptr_addr, "dataptr");
            llvm::Value* element_addr =
                builder.CreateInBoundsGEP(memory, { dataptr }, "element_addr");
            llvm::Value* element = builder.CreateLoad(element_addr, "element");
            llvm::Value* cmp = builder.CreateICmpEQ(element, builder.getInt8(0));
            llvm::BasicBlock* loop_body_block =
                llvm::BasicBlock::Create(context, "loop_body", jit_fn);
            llvm::BasicBlock* loop_exit_block =
                llvm::BasicBlock::Create(context, "loop_exit", jit_fn);
            builder.CreateCondBr(cmp, loop_exit_block, loop_body_block);
            open_bracket_stack.push(std::make_pair(loop_body_block, loop_exit_block));
            // This specifies that following created instructions should be appended to
            // the end of the specified block.
            builder.SetInsertPoint(loop_body_block);
            break;
        }
        case ']':
        {
            BracketBlocks blocks = open_bracket_stack.top();
            open_bracket_stack.pop();
            llvm::Value* dataptr = builder.CreateLoad(dataptr_addr, "dataptr");
            llvm::Value* element_addr =
                builder.CreateInBoundsGEP(memory, { dataptr }, "element_addr");
            llvm::Value* element = builder.CreateLoad(element_addr, "element");
            llvm::Value* cmp = builder.CreateICmpNE(element, builder.getInt8(0));
            builder.CreateCondBr(cmp, blocks.first, blocks.second);
            // This specifies that following created instructions should be appended to
            // the end of the specified block.
            builder.SetInsertPoint(blocks.second);
            break;
        }
        default:
            assert(0 && "unreachable!");
        }
    }

    builder.CreateRetVoid();
    return jit_fn;
}

void llvm_jit(const BFProgram& p)
{
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module(new llvm::Module("bf_module", context));

    // Add a declaration for external functions used in the JITed code. We use
    // putchar and getchar for I/O.
    llvm::Type* int32_type = llvm::Type::getInt32Ty(context);
    llvm::Function* putchar_fn =
        llvm::Function::Create(llvm::FunctionType::get(int32_type, { int32_type }, false),
                               llvm::Function::ExternalLinkage, "putchar", module.get());
    llvm::Function* getchar_fn =
        llvm::Function::Create(llvm::FunctionType::get(int32_type, {}, false),
                               llvm::Function::ExternalLinkage, "getchar", module.get());

    // Compile the BF program to LLVM IR.
    llvm::Function* jit_fn = emit_jit_function(p, module.get(), putchar_fn, getchar_fn);

    llvm::verifyFunction(*jit_fn);

    // Optimize the emitted LLVM IR.
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    llvm::PassManagerBuilder pm_builder;
    pm_builder.OptLevel = 3;
    pm_builder.SizeLevel = 0;

    llvm::legacy::FunctionPassManager function_pm(module.get());
    llvm::legacy::PassManager module_pm;
    pm_builder.populateFunctionPassManager(function_pm);
    pm_builder.populateModulePassManager(module_pm);

    function_pm.doInitialization();
    function_pm.run(*jit_fn);
    module_pm.run(*module);

    // JIT the optimized LLVM IR to native code and execute it.
    BrainfuckJIT jit;
    module->setDataLayout(jit.getTargetMachine().createDataLayout());
    jit.addModule(std::move(module));
    using JitFuncType = void (*)(void);
    JitFuncType jit_func_ptr =
        reinterpret_cast<JitFuncType>(jit.getSymbolAddress(JIT_FUNC_NAME));
    assert(jit_func_ptr && "Failed to codegen function");
    jit_func_ptr();
}

int main(int argc, const char** argv)
{
    if (argc == 2)
    {
        std::ifstream file(argv[1]);
        BFProgram program = parse_from_stream(file);
        llvm_jit(program);
    }

    return 0;
}