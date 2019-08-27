#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>

using namespace llvm;
using namespace clang;
using namespace clang::tooling;

#if LLVM_VERSION_MAJOR < 8
static PresumedLoc PrintDifference(raw_ostream &OS, const SourceManager &SM,
                                   SourceLocation Loc, PresumedLoc Previous)
{
    if (Loc.isFileID())
    {
        PresumedLoc PLoc = SM.getPresumedLoc(Loc);

        if (PLoc.isInvalid())
        {
            OS << "<invalid sloc>";
            return Previous;
        }

        if (Previous.isInvalid() ||
            strcmp(PLoc.getFilename(), Previous.getFilename()) != 0)
        {
            OS << PLoc.getFilename() << ':' << PLoc.getLine() << ':' << PLoc.getColumn();
        }
        else if (Previous.isInvalid() || PLoc.getLine() != Previous.getLine())
        {
            OS << "line" << ':' << PLoc.getLine() << ':' << PLoc.getColumn();
        }
        else
        {
            OS << "col" << ':' << PLoc.getColumn();
        }
        return PLoc;
    }
    auto PrintedLoc = PrintDifference(OS, SM, SM.getExpansionLoc(Loc), Previous);

    OS << " <Spelling=";
    PrintedLoc = PrintDifference(OS, SM, SM.getSpellingLoc(Loc), PrintedLoc);
    OS << '>';
    return PrintedLoc;
}
#endif

class CommentsExtractorConsumer : public clang::ASTConsumer
{
public:
    explicit CommentsExtractorConsumer(ASTContext *Context) {}

    void HandleTranslationUnit(clang::ASTContext &Context) override
    {
        auto comments = Context.getRawCommentList().getComments();
        for (auto comment : comments)
        {
            outs() << "Comment: < ";
            outs() << comment->getBriefText(Context);
            // outs() << comment->getRawText(Context.getSourceManager());
            // DiagnosticsEngine Diags(new DiagnosticIDs, new DiagnosticOptions);
            // outs() << comment->getFormattedText(Context.getSourceManager(), Diags);
            outs() << " > at ";
#if LLVM_VERSION_MAJOR < 8
            outs() << printCommentToString(comment, Context.getSourceManager());
#else
            comment->getSourceRange().print(outs(), Context.getSourceManager());
#endif
            outs() << "\n\n";
        }
    }

private:
#if LLVM_VERSION_MAJOR < 8
    std::string printCommentToString(const RawComment *RC, const SourceManager &SM)
    {
        SourceLocation B = RC->getSourceRange().getBegin();
        SourceLocation E = RC->getSourceRange().getEnd();
        std::string S;
        llvm::raw_string_ostream OS(S);
        OS << '<';
        auto PrintedLoc = PrintDifference(OS, SM, B, {});
        if (B != E)
        {
            OS << ", ";
            PrintDifference(OS, SM, E, PrintedLoc);
        }
        OS << '>';
        return OS.str();
    }
#endif
};

class CommentsExtractorAction : public clang::ASTFrontendAction
{
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &Compiler, llvm::StringRef InFile)
    {
        return std::unique_ptr<clang::ASTConsumer>(
            new CommentsExtractorConsumer(&Compiler.getASTContext()));
    }
};

static llvm::cl::OptionCategory CommentsExtractorCategory("comments-extractor options");

int main(int argc, const char **argv)
{
    CommonOptionsParser OptionsParser(argc, argv, CommentsExtractorCategory);
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<CommentsExtractorAction>().get());
}