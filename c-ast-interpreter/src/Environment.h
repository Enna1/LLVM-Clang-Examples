#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include <vector>
#include <iostream>//std::cin
#include <cstdlib> //std::malloc
#include <cstring> //std::memset
#include <cstdint> //std::uintptr_t
#include "util.h"

class Environment
{
public:
    std::vector<StackFrame> stack_frames_;
    Heap heap_;
private:

    //Declartions to the built-in functions
    FunctionDecl* ffree_;
    FunctionDecl* fmalloc_;
    FunctionDecl* finput_;
    FunctionDecl* foutput_;

    //fuction process entry, here is main()
    FunctionDecl* entry_;
public:
    Environment(): stack_frames_(), heap_(), ffree_(nullptr), fmalloc_(nullptr), 
        finput_(nullptr), foutput_(nullptr), entry_(nullptr) { }
    
    //initialize the Environment
    void Init(TranslationUnitDecl* unit)
    {
        //the stackframe of the entry function(i.e. main)
        stack_frames_.push_back(StackFrame());

        for (TranslationUnitDecl::decl_iterator i =unit->decls_begin(), 
            e = unit->decls_end(); i != e; ++i)
        {
            //handle function declare
            if (FunctionDecl * fdecl = dyn_cast<FunctionDecl>(*i))
            {
                if (fdecl->getName().equals("FREE")) ffree_ = fdecl;
                else if (fdecl->getName().equals("MALLOC")) fmalloc_ = fdecl;
                else if (fdecl->getName().equals("GET")) finput_ = fdecl;
                else if (fdecl->getName().equals("PRINT")) foutput_ = fdecl;
                else if (fdecl->getName().equals("main")) entry_ = fdecl;
            }
            //handle global variable declare
            HandleVarDecl(*i);
        }
    }

    FunctionDecl* getEntry()
    {
        return entry_;
    }

    void HandleDeclStmt(DeclStmt* DS)
    {
        for (auto i = DS->decl_begin(), e = DS->decl_end(); i != e; ++ i)
        {
            HandleVarDecl(*i);
        }
    }

    void HandleDeclRefExpr(DeclRefExpr* DRE)
    {
        /*
        StackFrame& sf_top = stack_frames_.back();
        ValueDecl* VD = DRE->getDecl();
        if (sf_top.hasDeclared(VD))
        {
            sf_top.push(DRE, sf_top.get(VD));
        }
        */
    }

    void HandleCastExpr(CastExpr* CE)
    {
        if (isa<ImplicitCastExpr>(CE))
        {
            stack_frames_.back().push(CE, EvaluateExpr(CE->IgnoreImpCasts()));
            //stack_frames_.back().push(CE, EvaluateExpr(ICE));
        }
        else if (auto CSCE = dyn_cast<CStyleCastExpr>(CE))
        {
            stack_frames_.back().push(CE, EvaluateExpr(CSCE->getSubExpr()));
        }
    }

    void HandleArraySubscriptExpr(ArraySubscriptExpr* ASE)
    {
        if (auto DRE = 
            dyn_cast<DeclRefExpr>(ASE->getLHS()->IgnoreImpCasts()))
        {
            ValueDecl* VD = DRE->getDecl();
            int64_t index = EvaluateExpr(ASE->getRHS());
            StackFrame& sf_top = stack_frames_.back();
            if (size_t tmp = varDeclared(VD))
            {
                sf_top.push(ASE, stack_frames_[tmp-1].get(VD, index));
            }
        }
    }

    /*
     * UnaryExprOrTypeTrait
     * see https://clang.llvm.org/doxygen/namespaceclang.html#a5d73f06594a5ccb763a726bed94a541f
     */
    void HandleUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr* UETT)
    {
        switch(UETT->getKind())
        {
            //for now only support "sizeof"
            case UETT_SizeOf:
            {
                if (UETT->getArgumentType()->isIntegerType())
                {
                    stack_frames_.back().push(UETT, sizeof(int64_t));
                }
                else if (UETT->getArgumentType()->isPointerType())
                {
                    stack_frames_.back().push(UETT, sizeof(int64_t*));
                }
                break;
            }
            default:
                break;
        }
    }
    
    void HandleUnaryOp(UnaryOperator* UO)
    {
        switch(UO->getOpcode())
        {
            case UO_Minus:
            {
                if (Expr* E = UO->getSubExpr())
                {
                    stack_frames_.back().push(UO, -EvaluateExpr(E));
                }
                break;
            }
            case UO_Deref:
            {
                int64_t addr = EvaluateExpr(UO->getSubExpr());
                int64_t* ptr = heap_.get(addr);
                stack_frames_.back().push(UO, *ptr);
                break;
            }
            default:
                break;
        }
    }

    void HandleBinOp(BinaryOperator* BO)
    {
        Expr* lhs = BO->getLHS();
        Expr* rhs = BO->getRHS();            
        switch(BO->getOpcode())
        {
            case BO_Assign: // "="
            {
                //TODO: refactor
                if (auto DRE = dyn_cast<DeclRefExpr>(lhs))
                {
                    ValueDecl* VD = DRE->getDecl();
                    StackFrame& sf_top = stack_frames_.back();
                    if (size_t tmp = varDeclared(VD))
                    {
                        int64_t val = EvaluateExpr(rhs);
                        stack_frames_[tmp-1].get(VD) = val;
                        sf_top.push(BO, val);
                    }
                }
                else if (auto UO = dyn_cast<UnaryOperator>(lhs))
                {
                    if (UO->getOpcode() == UO_Deref)
                    {
                        int64_t addr = EvaluateExpr(UO->getSubExpr());
                        int64_t val = EvaluateExpr(rhs);
                        int64_t* ptr = heap_.get(addr);
                        *ptr = val;
                        stack_frames_.back().push(BO, val);
                    }
                }
                else if (auto ASE = dyn_cast<ArraySubscriptExpr>(lhs))
                {
                    if (auto DRE = 
                        dyn_cast<DeclRefExpr>(ASE->getLHS()->IgnoreImpCasts()))
                    {
                        ValueDecl* VD = DRE->getDecl();
                        int64_t index = EvaluateExpr(ASE->getRHS());
                        int64_t val = EvaluateExpr(rhs);
                        if (size_t tmp = varDeclared(VD))
                        {
                            stack_frames_[tmp-1].get(VD, index) = val;
                        }
                        stack_frames_.back().push(BO, val);
                    }
                }
                break;
            }
            case BO_Add: // "+"
            {
                //handle addition for ptr, e.g. int *p;, p+1
                if (lhs->getType().getTypePtr()->isPointerType())
                {
                    int64_t addr = EvaluateExpr(lhs);
                    int64_t* ptr = heap_.get(addr);
                    int64_t val = EvaluateExpr(rhs);
                    ptr += val;
                    heap_.push(reinterpret_cast<std::uintptr_t>(ptr), ptr);

                    stack_frames_.back().push(BO, 
                        reinterpret_cast<std::uintptr_t>(ptr));
                }
                else
                {
                    stack_frames_.back().push(BO, 
                        EvaluateExpr(lhs) + EvaluateExpr(rhs));
                }
                break;
            }
            case BO_Sub: // "-"
            {
                //handle addition for ptr, e.g. int *p;, p+1
                if (lhs->getType().getTypePtr()->isPointerType())
                {
                    int64_t addr = EvaluateExpr(lhs);
                    int64_t* ptr = heap_.get(addr);
                    int64_t val = EvaluateExpr(rhs);
                    ptr -= val;
                    heap_.push(reinterpret_cast<std::uintptr_t>(ptr), ptr);

                    stack_frames_.back().push(BO, 
                        reinterpret_cast<std::uintptr_t>(ptr));
                }
                else
                {
                    stack_frames_.back().push(BO, 
                        EvaluateExpr(lhs) - EvaluateExpr(rhs));
                }
                break;
            }
            case BO_Mul: // "*"
            {
                stack_frames_.back().push(BO, 
                    EvaluateExpr(lhs) * EvaluateExpr(rhs));
                break;
            }
            case BO_Div: // "/"
            {
                stack_frames_.back().push(BO, 
                    EvaluateExpr(lhs) / EvaluateExpr(rhs));
                break;
            }
            case BO_LT: // "<"
            {
                stack_frames_.back().push(BO, 
                    EvaluateExpr(lhs) < EvaluateExpr(rhs));
                break;
            }
            case BO_GT: // ">"
            {
                stack_frames_.back().push(BO, 
                    EvaluateExpr(lhs) > EvaluateExpr(rhs));
                break;
            }
            case BO_EQ: // "=="
            {
                stack_frames_.back().push(BO, 
                    EvaluateExpr(lhs) == EvaluateExpr(rhs));
            }
            default:
                break;
        }
    }

    void HandleCallExpr(CallExpr* CE)
    {
        if (FunctionDecl* FD = CE->getDirectCallee())
        {
            if (FD == finput_)
            {
                int64_t input = 0;
                llvm::errs() << "Please Input an Integer Value : ";
                std::cin >> input;
                stack_frames_.back().push(CE, input);
            }
            else if (FD == foutput_)
            {
                assert(CE->getNumArgs() == 1);
                if (CE->getArg(0)->IgnoreImpCasts()->getType().getTypePtr()
                    ->isCharType())
                {
                    llvm::errs() << static_cast<char>(EvaluateExpr(
                        CE->getArg(0))) << "\n";
                }
                // default treated as IntegerType
                else 
                {
                   llvm::errs() << EvaluateExpr(CE->getArg(0)) << "\n";
                }
            }
            else if (FD == fmalloc_)
            {
                assert(CE->getNumArgs() == 1);
                int64_t malloc_size = EvaluateExpr(CE->getArg(0));
                int64_t* ptr = (int64_t*)std::malloc(malloc_size);
                std::memset(ptr, 0, malloc_size);
                stack_frames_.back().push(CE, reinterpret_cast<std::uintptr_t>(ptr));
                heap_.push(reinterpret_cast<std::uintptr_t>(ptr), ptr);
            }
            else if (FD == ffree_)
            {
                assert(CE->getNumArgs() == 1);
                int64_t addr = EvaluateExpr(CE->getArg(0));
                int64_t* ptr = heap_.get(addr);
                std::free(ptr);
            }
            else //handle user-defined function
            {
                std::vector<int64_t> v;
                for (auto i = CE->arg_begin(), e = CE->arg_end(); 
                    i != e; ++i)
                {
                    v.push_back(EvaluateExpr(*i));
                }
                stack_frames_.push_back(StackFrame());
                StackFrame& sf_top = stack_frames_.back();
                for (auto i = FD->param_begin(), e = FD->param_end(); 
                    i != e; ++i)
                {
                    sf_top.push(*i, v[i-FD->param_begin()]);
                }
            }
        }
    }

    void HandleVarDecl(Decl* D)
    {
        if(auto VD = dyn_cast<VarDecl>(D))
        {
            // use same structure (StackFrame.VarMap) to 
            // store Integer and Pointer
            if (VD->getType().getTypePtr()->isIntegerType() || 
                VD->getType().getTypePtr()->isPointerType())
            {
                if (VD->hasInit())
                {
                    stack_frames_.back().push(VD, EvaluateExpr(VD->getInit()));
                }
                else
                {
                    stack_frames_.back().push(VD, 0);
                }    
            }
            else if (VD->getType().getTypePtr()->isArrayType())
            {
                // for now only handle ConstantArray
                if (auto CAT = 
                    dyn_cast<ConstantArrayType>(VD->getType().getTypePtr()))
                {
                    int64_t array_size = CAT->getSize().getSExtValue();
                    if (VD->hasInit())
                    {
                        //llvm::errs() << "TODO: hanlde declared with init\n";
                    }
                    else
                    {
                        stack_frames_.back().push_array(VD, array_size);
                    }
                }
            }
        }
    }

    void HandleReturnStmt(ReturnStmt* RS)
    {
        stack_frames_.back().set_func_ret(EvaluateExpr(RS->getRetValue()));
    }

    int64_t EvaluateExpr(Expr* E)
    {
        Expr* e = E->IgnoreImpCasts();
        if (stack_frames_.back().hasDeclared(e))
        {
            return stack_frames_.back().get(e);
        }
        else if (auto IL = dyn_cast<IntegerLiteral>(e))
        {
            return IL->getValue().getSExtValue();
        }
        else if (auto CL = dyn_cast<CharacterLiteral>(e))
        {
            return CL->getValue();
        }
        else if (auto DRE = dyn_cast<DeclRefExpr>(e))
        {
            ValueDecl* VD = DRE->getDecl();
            if (size_t tmp = varDeclared(VD))
            {
                return stack_frames_[tmp-1].get(VD);
            }
        }
        else if (auto BO = dyn_cast<BinaryOperator>(e))
        {
            HandleBinOp(BO);
            if (stack_frames_.back().hasDeclared(e))
            {
                return stack_frames_.back().get(e);
            }
        }
        else if (auto UO = dyn_cast<UnaryOperator>(e))
        {
            HandleUnaryOp(UO);
            if (stack_frames_.back().hasDeclared(e))
            {
                return stack_frames_.back().get(e);
            }
        }
        else if (auto UETT = dyn_cast<UnaryExprOrTypeTraitExpr>(e))
        {
            HandleUnaryExprOrTypeTraitExpr(UETT);
            if (stack_frames_.back().hasDeclared(e))
            {
                return stack_frames_.back().get(e);
            }
        }       
        else if (auto PE = dyn_cast<ParenExpr>(e))
        {
            return EvaluateExpr(PE->getSubExpr());
        }
        else if (auto CSCE = dyn_cast<CStyleCastExpr>(e))
        {
            return EvaluateExpr(CSCE->getSubExpr());
        }
        return 0;
    }

    /*
     * if a variable has declared in a StackFrame, 
     * return 1 + the index of the StackFrame in stack_frames_, else return 0
     */
    size_t varDeclared(const Decl* D)
    {
        for (auto i = stack_frames_.rbegin(), e = stack_frames_.rend(); 
            i != e; ++i)
        {
            if (i->hasDeclared(D))
            {
                return (e - i);
            }
        }
        return 0;
    }

};