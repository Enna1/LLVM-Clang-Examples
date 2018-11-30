#include "clang/AST/ASTConsumer.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

#include "Environment.h"

class InterpreterVisitor : 
    public EvaluatedExprVisitor<InterpreterVisitor>
{
public:
    explicit InterpreterVisitor(const ASTContext &context, Environment *env)
            : EvaluatedExprVisitor(context), env_(env) {}

    virtual ~InterpreterVisitor() = default;
/*
    virtual void VisitDeclRefExpr(DeclRefExpr* DRE)
    {
        VisitStmt(DRE);
        env_->HandleDeclRefExpr(DRE);
    }

    virtual void VisitCastExpr(CastExpr* CE)
    {
        VisitStmt(CE);
        env_->HandleCastExpr(CE);
    }
*/
    void VisitStmtWrap(Stmt* S)
    {
        if (!(env_->stack_frames_.back().is_set_func_ret()))
        {
            VisitStmt(S);
        }
    }

    virtual void VisitArraySubscriptExpr(ArraySubscriptExpr* ASE)
    {
        VisitStmtWrap(ASE);
        env_->HandleArraySubscriptExpr(ASE);
    }

    virtual void VisitReturnStmt(ReturnStmt* RS)
    {
        if (!(env_->stack_frames_.back().is_set_func_ret()))
        {
            VisitStmtWrap(RS);
            env_->HandleReturnStmt(RS);
        }
    }

    virtual void VisitDeclStmt(DeclStmt* DS)
    {
        VisitStmtWrap(DS);
        env_->HandleDeclStmt(DS);
    }
    
    virtual void VisitUnaryOperator(UnaryOperator* UO)
    {
        VisitStmtWrap(UO);
        env_->HandleUnaryOp(UO);
    }

    virtual void VisitBinaryOperator(BinaryOperator* BO)
    {
        VisitStmtWrap(BO);
        env_->HandleBinOp(BO);
    }

    virtual void VisitCallExpr(CallExpr* CE)
    {
        VisitStmtWrap(CE);
        env_->HandleCallExpr(CE);
        if (FunctionDecl* FD = CE->getDirectCallee())
        {
            if ((!FD->getName().equals("GET")) && 
                (!FD->getName().equals("PRINT")) && 
                (!FD->getName().equals("MALLOC")) && 
                (!FD->getName().equals("FREE")))
            {
                VisitStmtWrap(FD->getBody());
                int ret = env_->stack_frames_.back().func_ret();
                env_->stack_frames_.pop_back();
                env_->stack_frames_.back().push(CE, ret);
            }
        }
        
    }

    virtual void VisitIfStmt(IfStmt* IS)
    {
        if (env_->EvaluateExpr(IS->getCond()) != 0)
        {
            Visit(IS->getThen());
        }
        else
        {
            if (IS->getElse())
                Visit(IS->getElse());
        }
        env_->stack_frames_.back().popExpr();
    }

    virtual void VisitWhileStmt(WhileStmt* WS)
    {
        while(env_->EvaluateExpr(WS->getCond()) != 0)
        {
            VisitStmtWrap(WS->getBody());
            env_->stack_frames_.back().popExpr();
        }
    }

    virtual void VisitForStmt(ForStmt* FS)
    {
        /*
         * Note:
         * VisitStmt() walks all of the children of the statement or expression
         * So we use Visit() for FS->getInit() here
         */
        if (FS->getInit())
        {
            Visit(FS->getInit());
        }

        while (env_->EvaluateExpr(FS->getCond()) != 0)
        {
            VisitStmtWrap(FS->getBody());
            if (FS->getInc())
            {
                Visit(FS->getInc());
            }
            env_->stack_frames_.back().popExpr();
        }
    }
/*
    virtual void VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr* UETT)
    {
        env_->HandleUnaryExprOrTypeTraitExpr(UETT);
    }*/
private:
    Environment *env_;
};

class InterpreterConsumer : public ASTConsumer
{
public:
    explicit InterpreterConsumer(const ASTContext& context) :
            env_(), visitor_(context, &env_) { }

    ~InterpreterConsumer() override = default;

    void HandleTranslationUnit(clang::ASTContext& Context) override
    {
        TranslationUnitDecl *decl = Context.getTranslationUnitDecl();
        env_.Init(decl);

        FunctionDecl *entry = env_.getEntry();
        visitor_.VisitStmt(entry->getBody());
    }

private:
    Environment env_;
    InterpreterVisitor visitor_;
};

class InterpreterClassAction : public ASTFrontendAction
{
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
            clang::CompilerInstance &Compiler, llvm::StringRef InFile)
    {
        return std::unique_ptr<clang::ASTConsumer>(
                new InterpreterConsumer(Compiler.getASTContext()));
    }
};

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        clang::tooling::runToolOnCode(new InterpreterClassAction, argv[1], "input.c");
    }
}