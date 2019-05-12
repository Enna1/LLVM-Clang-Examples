#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Transforms/Utils/UnifyFunctionExitNodes.h>

class UnifyFunctionExits : public llvm::ModulePass
{
public:
    static char ID;
    UnifyFunctionExits() : ModulePass(ID) {}
    llvm::StringRef getPassName() const
    {
        return "Call FunctionPass UnifyFunctionExitNodesunify to ensure that functions "
               "have at most one return and one unwind instruction ";
    }

    virtual bool runOnModule(llvm::Module& M)
    {
        UnifyFunctionExit(M);
        return true;
    }

    inline void UnifyFunctionExit(llvm::Module& module)
    {
        for (auto i = module.begin(), e = module.end(); i != e; ++i)
        {
            const llvm::Function& fn = *i;
            if (fn.isDeclaration())
                continue;
            getUnifyExit(fn)->runOnFunction(const_cast<llvm::Function&>(fn));
            
            // We can call getUnifyExit(fn)->getReturnBlock(), getUnwindBlock(),
            // getUnreachableBlock() to get the new single (or nonexistent) return,
            // unwind, or unreachable basic blocks of the function.
        }
    }

    inline llvm::UnifyFunctionExitNodes* getUnifyExit(const llvm::Function& fn)
    {
        assert(!fn.isDeclaration() && "external function does not have DF");
        return &getAnalysis<llvm::UnifyFunctionExitNodes>(
            const_cast<llvm::Function&>(fn));
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const
    {
        AU.addRequired<llvm::UnifyFunctionExitNodes>();
    }
};

char UnifyFunctionExits::ID = 0;
