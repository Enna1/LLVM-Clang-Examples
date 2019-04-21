#include "FooPass.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/SourceMgr.h>  // SMDiagnostic
#include <llvm/Transforms/IPO.h>     // createCalledValuePropagationPass
using namespace llvm;

#if LLVM_VERSION_MAJOR >= 4
static ManagedStatic<LLVMContext> GlobalContext;
static LLVMContext &getGlobalContext()
{
    return *GlobalContext;
}
#endif

static RegisterPass<FooPass> X("FooPass", "A pass that implements something");

static cl::opt<std::string> InputFilename(cl::Positional, cl::desc("<filename>.bc"),
                                          cl::init(""));

int main(int argc, char **argv)
{
    LLVMContext &Context = getGlobalContext();
    SMDiagnostic Err;
    // Parse the command line to read the Inputfilename
    cl::ParseCommandLineOptions(argc, argv, "FooPass.\n");

    // Load the input module
    std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
    if (!M)
    {
        Err.print(argv[0], errs());
        return 1;
    }

    llvm::legacy::PassManager Passes;
    /// Call CalledValuePropagationPass
    Passes.add(llvm::createCalledValuePropagationPass());
    /// Call FooPass
    Passes.add(new FooPass());
    Passes.run(*M.get());
}