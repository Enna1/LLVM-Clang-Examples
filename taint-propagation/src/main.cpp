#include "TaintPropagation.h"
#include <llvm/Bitcode/BitcodeWriter.h>  // WriteBitcodeToFile
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/SourceMgr.h>  // SMDiagnostic
#include <llvm/Transforms/Utils.h>   // createPromoteMemoryToRegisterPass
using namespace llvm;

#if LLVM_VERSION_MAJOR >= 4
static ManagedStatic<LLVMContext> GlobalContext;
static LLVMContext &getGlobalContext()
{
    return *GlobalContext;
}
#endif

// From LLVM 5.0, when -O0 passed to clang, clang will add optnone attribute to
// each function, which prevents further optimizations afterwards including mem2reg pass.
// To prevent that, we can add -Xclang -disable-O0-optnone options to clang, or add the
// following pass to PassManager.
#if LLVM_VERSION_MAJOR >= 5
struct DisableOptnonePass : public FunctionPass
{
    static char ID;
    DisableOptnonePass() : FunctionPass(ID) {}
    bool runOnFunction(Function &F) override
    {
        if (F.hasFnAttribute(Attribute::OptimizeNone))
        {
            F.removeFnAttr(Attribute::OptimizeNone);
        }
        return true;
    }
};
char DisableOptnonePass::ID = 0;
#endif

static RegisterPass<TaintPropagationLegacyPass> X(
    "TaintPropagationLegacyPass",
    "A pass that implements dataflow-based taint propagation.");

static cl::opt<std::string> InputFilename(cl::Positional, cl::desc("<filename>.bc"),
                                          cl::init(""));
static cl::opt<std::string> OutputFilename("o", cl::desc("Specify output filename"),
                                           cl::value_desc("filename"));

int main(int argc, char **argv)
{
    LLVMContext &Context = getGlobalContext();
    SMDiagnostic Err;
    // Parse the command line to read the Inputfilename
    cl::ParseCommandLineOptions(argc, argv, "TaintPropagationLegacyPass.\n");

    // Load the input module
    std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
    if (!M)
    {
        Err.print(argv[0], errs());
        return 1;
    }

    llvm::legacy::PassManager PassMgr;

    // PassRegistry &Registry = *PassRegistry::getPassRegistry();
    // initializeUnifyFunctionExitNodesPass(Registry);

    // Remove functions' optnone attribute
    PassMgr.add(new DisableOptnonePass());
    // Transform it to SSA
    PassMgr.add(llvm::createPromoteMemoryToRegisterPass());
    // Call TaintPropagationLegacyPass
    PassMgr.add(new TaintPropagationLegacyPass());
    PassMgr.run(*M.get());

    if (!OutputFilename.empty())
    {
        std::error_code EC;
        raw_fd_ostream FOS(OutputFilename, EC, sys::fs::F_None);
        // WriteBitcodeToFile(*M.get(), FOS);
        M->print(FOS, nullptr);
    }
}