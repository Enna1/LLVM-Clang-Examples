#include <map>
#include <memory>
#include "clang/AST/Decl.h" //define clang::Decl

class StackFrame
{
//<ptr to integer variable declared, corressponding value>
using VarMap = std::unordered_multimap<const Decl*, int64_t>;
//<ptr to expression, evaluated value>
using ExprMap = std::unordered_map<const Expr*, int64_t>;

struct FuncRet
{
    bool is_set;
    int64_t val;    
};

public:
    StackFrame():varmap_(), exprmap_(), func_ret_{false, 0} { }
    ~StackFrame() { }
    
    //operations on varmap_
    void push(Decl* D, int64_t val)
    {
        varmap_.insert({D, val});
    }

    void push_array(Decl* D, int64_t size)
    {
        for (int64_t i = 0; i < size; ++i)
        {
            push(D, 0);
        }
    }

    int64_t& get(const Decl* D)
    {
        assert(varmap_.count(D) == 1);
        return get(D, 0);
    }

    int64_t& get(const Decl* D, int64_t i)
    {
        assert((i >= 0) && (static_cast<int64_t>(varmap_.count(D)) > i));
        auto range = varmap_.equal_range(D);
        int64_t tmp = 0;
        for (auto p = range.first; p != range.second; ++p, ++tmp)
        {
            if (tmp == i)
            {
                return p->second;
            }
        }
        //error
        return range.first->second;
    }

    bool hasDeclared(const Decl* D)
    {
        return (varmap_.find(D) != varmap_.end());
    }

    //operations for exprmap_
    void push(Expr* E, int64_t val)
    {
        exprmap_.emplace(std::make_pair(E, val));
    }

    void popExpr()
    {
        exprmap_.clear();
    }

    bool hasDeclared(const Expr* E) //function name not exact
    {
        return (exprmap_.find(E) != exprmap_.end());
    }

    int64_t& get(const Expr* E)
    {
        assert(hasDeclared(E));
        return exprmap_[E];
    }

    //operations related to function call
    void set_func_ret(int64_t ret)
    {
        func_ret_.is_set = true;
        func_ret_.val = ret;
    }

    bool is_set_func_ret()
    {
        return func_ret_.is_set;
    }

    int64_t func_ret()
    {
        return func_ret_.val;
    }

private:
    VarMap varmap_;
    ExprMap exprmap_;
    FuncRet func_ret_;
    
};

class Heap
{
//<addr cast to int, addr> >
using VarMap = std::unordered_map<int64_t, int64_t*>;

public:
    Heap(): varmap_() {}
    ~Heap() {}
    void push(int64_t addrcast, int64_t* addr)
    {
        varmap_.emplace(std::make_pair(addrcast, addr));
    }
    
    bool hasDeclared(int64_t addrcast)
    {
        return (varmap_.find(addrcast) != varmap_.end());
    }

    int64_t* get(int64_t addrcast)
    {
        assert(hasDeclared(addrcast));
        return varmap_[addrcast];
    }

private:
    VarMap varmap_;

};