#ifndef BREAKCONSTANTEXPR_H
#define BREAKCONSTANTEXPR_H

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>

class BreakConstantExpr : public llvm::ModulePass
{
public:
    static char ID;
    BreakConstantExpr() : llvm::ModulePass(ID) {}
    llvm::StringRef getPassName() const
    {
        return "Convert constant expressions into regular instructions.";
    }
    virtual bool runOnModule(llvm::Module &M);
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const
    {
        // This pass does not modify the control-flow graph of the function
        AU.setPreservesCFG();
    }
};

#endif
