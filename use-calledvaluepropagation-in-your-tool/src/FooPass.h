#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/LLVMContext.h>  // LLVMContext::MD_callees
#include <llvm/IR/Metadata.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
using namespace llvm;  // This is a dumb operation

class FooPass : public FunctionPass
{
public:
    static char ID;

private:
public:
    FooPass() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override
    {
        for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i)
        {
            Instruction *I = &(*i);
            if (auto *MD = I->getMetadata(LLVMContext::MD_callees))
            {
                for (auto &Op : MD->operands())
                {
                    Function *Callee = mdconst::extract_or_null<Function>(Op);
                    if (Callee)
                        errs() << Callee->getName() << ", ";
                }
                errs() << "\n";
            }
        }
        return false;
    }

private:
};

char FooPass::ID = 0;
