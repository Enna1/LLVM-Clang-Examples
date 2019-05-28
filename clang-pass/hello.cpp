#include <llvm/IR/Function.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
using namespace llvm;

namespace
{
struct Hello : public FunctionPass
{
    static char ID;  // Pass identification, replacement for typeid
    Hello() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override
    {
        errs() << "Hello: ";
        errs().write_escaped(F.getName()) << '\n';
        return false;
    }

    // We don't modify the program, so we preserve all analyses.
    void getAnalysisUsage(AnalysisUsage &AU) const override
    {
        AU.setPreservesAll();
    }
};
}  // namespace

char Hello::ID = 0;

static void registerHelloPass(const PassManagerBuilder &, legacy::PassManagerBase &PM)
{
    PM.add(new Hello());
}
static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                                             registerHelloPass);