#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/IR/Function.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#if LLVM_VERSION_MAJOR >= 4
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>

#else
#include <llvm/Bitcode/ReaderWriter.h>
#endif

#include "PointTo.h"

using namespace llvm;

#if LLVM_VERSION_MAJOR >= 4
static ManagedStatic<LLVMContext> GlobalContext;
static LLVMContext &getGlobalContext()
{
    return *GlobalContext;
}
#endif

/* In LLVM 5.0, when  -O0 passed to clang , the functions generated with clang
 * will have optnone attribute which would lead to some transform passes
 * disabled, like mem2reg.
 */
#if LLVM_VERSION_MAJOR == 5
struct EnableFunctionOptPass : public FunctionPass
{
    static char ID;
    EnableFunctionOptPass() : FunctionPass(ID) {}
    bool runOnFunction(Function &F) override
    {
        if (F.hasFnAttribute(Attribute::OptimizeNone))
        {
            F.removeFnAttr(Attribute::OptimizeNone);
        }
        return true;
    }
};

char EnableFunctionOptPass::ID = 0;
#endif

static RegisterPass<PointToPass> X(
    "PointToPass", "Point-to Analysis via Dataflow, print function call instruction");

static cl::opt<std::string> InputFilename(cl::Positional, cl::desc("<filename>.bc"),
                                          cl::init(""));

int main(int argc, char **argv)
{
    LLVMContext &Context = getGlobalContext();
    SMDiagnostic Err;
    // Parse the command line to read the Inputfilename
    cl::ParseCommandLineOptions(argc, argv, "PointToPass.\n");

    // Load the input module
    std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
    if (!M)
    {
        Err.print(argv[0], errs());
        return 1;
    }

    llvm::legacy::PassManager Passes;
#if LLVM_VERSION_MAJOR == 5
    Passes.add(new EnableFunctionOptPass());
#endif
    /// Transform it to SSA
    Passes.add(llvm::createPromoteMemoryToRegisterPass());

    /// Your pass to print Function and Call Instructions
    Passes.add(new PointToPass());
    Passes.run(*M.get());
}
