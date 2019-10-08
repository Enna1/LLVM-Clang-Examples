#include "Dataflow.h"
#include <algorithm>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using namespace llvm;

using ValueSet = std::unordered_set<Value *>;
using FunctionSet = std::unordered_set<Function *>;
using PointToMap = std::unordered_map<Value *, ValueSet>;

struct CallInstCmp
{
    bool operator()(const CallInst *lhs, const CallInst *rhs)
    {
        return (lhs->getDebugLoc().getLine() < rhs->getDebugLoc().getLine());
    }
};

struct PointToInfo
{
    PointToMap pt_map;        // point-to set of value
    PointToMap field_pt_map;  // point-to set of value's field
    PointToInfo() : pt_map(), field_pt_map() {}
    PointToInfo(const PointToInfo &other)
        : pt_map(other.pt_map), field_pt_map(other.field_pt_map)
    {
    }
    bool operator==(const PointToInfo &other) const
    {
        return (pt_map == other.pt_map) && (field_pt_map == other.field_pt_map);
    }
    bool operator!=(const PointToInfo &other) const
    {
        return (pt_map != other.pt_map) || (field_pt_map != other.field_pt_map);
    }
    PointToInfo &operator=(const PointToInfo &other)  // copy assignment
    {
        pt_map = other.pt_map;
        field_pt_map = other.field_pt_map;
        return *this;
    }
};

inline raw_ostream &operator<<(raw_ostream &out, const PointToMap &v)
{
    out << "{ ";
    for (auto i = v.begin(), e = v.end(); i != e; ++i)
    {
        out << i->first->getName() << " " << i->first << " -> ";
        for (auto ii = i->second.begin(), ie = i->second.end(); ii != ie; ++ii)
        {
            if (ii != i->second.begin())
            {
                errs() << ", ";
            }
            out << (*ii)->getName() << " " << (*ii);
        }
        out << " ; ";
    }
    out << "}";
    return out;
}

inline raw_ostream &operator<<(raw_ostream &out, const PointToInfo &info)
{
    out << "pt_map : " << info.pt_map << "\n";
    out << "field_pt_map : " << info.field_pt_map << "\n";
    return out;
}

FunctionSet getValuePointToFunctions(Value *v, PointToInfo *dfval)
{
    FunctionSet ret;
    ValueSet worklist;
    if (auto *F = dyn_cast<Function>(v))
    {
        ret.insert(F);
        return ret;
    }

    if (dfval->pt_map.count(v))
    {
        worklist.insert(dfval->pt_map[v].begin(), dfval->pt_map[v].end());
    }

    while (!worklist.empty())
    {
        Value *i = *worklist.begin();
        worklist.erase(worklist.begin());
        if (auto *F = dyn_cast<Function>(i))
        {
            ret.insert(F);
        }
        else
        {
            worklist.insert(dfval->pt_map[i].begin(), dfval->pt_map[i].end());
        }
    }
    return ret;
}

class PointToVisitor : public DataflowVisitor<struct PointToInfo>
{
public:
    FunctionSet worklist_;
    std::map<CallInst *, FunctionSet, CallInstCmp> call_graph_;

public:
    PointToVisitor() : worklist_(), call_graph_() {}

    void merge(PointToInfo *dest, const PointToInfo &src) override
    {
        for (auto i = src.pt_map.begin(), e = src.pt_map.end(); i != e; ++i)
        {
            dest->pt_map[i->first].insert(i->second.begin(), i->second.end());
        }
        for (auto i = src.field_pt_map.begin(), e = src.field_pt_map.end(); i != e; ++i)
        {
            dest->field_pt_map[i->first].insert(i->second.begin(), i->second.end());
        }
    }

    void compDFVal(Instruction *inst, DataflowResult<PointToInfo>::Type *result) override
    {
        if (isa<IntrinsicInst>(inst))
        {
            if (auto *MCI = dyn_cast<MemCpyInst>(inst))
            {
                transferOnMemCpyInst(MCI, result);
            }
            else
            {
                (*result)[inst].second = (*result)[inst].first;
                return;
            }
        }
        else if (auto *PHI = dyn_cast<PHINode>(inst))
        {
            transferOnPhiNode(PHI, result);
        }
        else if (auto *CI = dyn_cast<CallInst>(inst))
        {
            transferOnCallInst(CI, result);
        }
        else if (auto *SI = dyn_cast<StoreInst>(inst))
        {
            transferOnStoreInst(SI, result);
        }
        else if (auto *LI = dyn_cast<LoadInst>(inst))
        {
            transferOnLoadInst(LI, result);
        }
        else if (auto *RI = dyn_cast<ReturnInst>(inst))
        {
            transferOnReturnInst(RI, result);
        }
        else if (auto *GEP = dyn_cast<GetElementPtrInst>(inst))
        {
            transferOnGetElementPtrInst(GEP, result);
        }
        else if (auto *BCI = dyn_cast<BitCastInst>(inst))
        {
            transferOnBitCastInst(BCI, result);
        }
        else
        {
            (*result)[inst].second = (*result)[inst].first;
        }
        // debug
        // errs() << inst;
        // inst->dump();
        // errs() << "in :  " << (*result)[inst].first << "\n";
        // errs() << "out : " << (*result)[inst].second << "\n";
    }

    void transferOnPhiNode(PHINode *PHI, DataflowResult<PointToInfo>::Type *result)
    {
        PointToInfo dfval = (*result)[PHI].first;
        dfval.pt_map[PHI].clear();
        for (Value *V : PHI->incoming_values())
        {
            if (isa<ConstantPointerNull>(V))
                continue;
            if (isa<Function>(V))
            {
                dfval.pt_map[PHI].insert(V);
            }
            else
            {
                ValueSet &v_set = dfval.pt_map[V];
                dfval.pt_map[PHI].insert(v_set.begin(), v_set.end());
            }
        }
        (*result)[PHI].second = dfval;
    }

    void transferOnCallInst(CallInst *CI, DataflowResult<PointToInfo>::Type *result)
    {
        PointToInfo dfval = (*result)[CI].first;

        FunctionSet callee_set = getValuePointToFunctions(CI->getCalledValue(), &dfval);
        call_graph_[CI].clear();
        call_graph_[CI].insert(callee_set.begin(), callee_set.end());

        // if callee function has definition, the dfval-out of this callinst will be set
        // in transferOnReturnInst when callee function is iterated. else the dfval-out is
        // set equal to dfval-in
        if (CI->getCalledFunction() && CI->getCalledFunction()->isDeclaration())
        {
            (*result)[CI].second = (*result)[CI].first;
            return;
        }

        for (auto i = callee_set.begin(), e = callee_set.end(); i != e; ++i)
        {
            Function *callee = *i;
            if (callee->isDeclaration())
                continue;
            // construct arg_map
            std::unordered_map<Value *, Argument *> arg_map;
            for (unsigned arg_i = 0; arg_i < CI->getNumArgOperands(); ++arg_i)
            {
                Value *caller_arg = CI->getArgOperand(arg_i);
                if (!caller_arg->getType()->isPointerTy())
                    continue;
                Argument *callee_arg = callee->arg_begin() + arg_i;
                arg_map.insert(std::make_pair(caller_arg, callee_arg));
            }
            // set dfval-in of callee function's entry point
            PointToInfo &callee_dfval_in = (*result)[&*inst_begin(callee)].first;
            PointToInfo old_callee_dfval_in = callee_dfval_in;
            PointToInfo tmp_dfval = (*result)[CI].first;
            if (arg_map.empty())
            {
                merge(&((*result)[CI].second), (*result)[CI].first);
                continue;
            }
            // replace caller arg with callee arg in pt_map and field_pt_map
            for (auto pi = tmp_dfval.pt_map.begin(), pe = tmp_dfval.pt_map.end();
                 pi != pe; ++pi)
            {
                for (auto ai = arg_map.begin(), ae = arg_map.end(); ai != ae; ++ai)
                {
                    if (pi->second.count(ai->first) && !isa<Function>(ai->first))
                    {
                        pi->second.erase(ai->first);
                        pi->second.insert(ai->second);
                    }
                }
            }
            for (auto pi = tmp_dfval.field_pt_map.begin(),
                      pe = tmp_dfval.field_pt_map.end();
                 pi != pe; ++pi)
            {
                for (auto ai = arg_map.begin(), ae = arg_map.end(); ai != ae; ++ai)
                {
                    if (pi->second.count(ai->first) && !isa<Function>(ai->first))
                    {
                        pi->second.erase(ai->first);
                        pi->second.insert(ai->second);
                    }
                }
            }
            for (auto ai = arg_map.begin(), ae = arg_map.end(); ai != ae; ++ai)
            {
                if (tmp_dfval.pt_map.count(ai->first))
                {
                    ValueSet v_set = tmp_dfval.pt_map[ai->first];
                    tmp_dfval.pt_map.erase(ai->first);
                    tmp_dfval.pt_map[ai->second].insert(v_set.begin(), v_set.end());
                }
                if (tmp_dfval.field_pt_map.count(ai->first))
                {
                    ValueSet v_set = tmp_dfval.field_pt_map[ai->first];
                    tmp_dfval.field_pt_map.erase(ai->first);
                    tmp_dfval.field_pt_map[ai->second].insert(v_set.begin(), v_set.end());
                }
            }
            for (auto ai = arg_map.begin(), ae = arg_map.end(); ai != ae; ++ai)
            {
                if (isa<Function>(ai->first))
                {
                    tmp_dfval.pt_map[ai->second].insert(ai->first);
                }
            }

            merge(&callee_dfval_in, tmp_dfval);
            if (old_callee_dfval_in != callee_dfval_in)
            {
                worklist_.insert(callee);
                // errs() << "[+]" << callee->getName()
                //        << " is insert to worklist, because of caller\n";
            }
        }
    }

    void transferOnReturnInst(ReturnInst *RI, DataflowResult<PointToInfo>::Type *result)
    {
        (*result)[RI].second = (*result)[RI].first;
        
        Function *callee = RI->getFunction();
        for (auto i = call_graph_.begin(), e = call_graph_.end(); i != e; ++i)
        {
            if (i->second.count(callee))
            {
                CallInst *CI = i->first;
                Function *caller = CI->getFunction();
                // construct arg_map
                std::unordered_map<Value *, Argument *> arg_map;
                for (unsigned arg_i = 0; arg_i < CI->getNumArgOperands(); ++arg_i)
                {
                    Value *caller_arg = CI->getArgOperand(arg_i);
                    if (!caller_arg->getType()->isPointerTy())
                        continue;
                    Argument *callee_arg = callee->arg_begin() + arg_i;
                    arg_map.insert(std::make_pair(caller_arg, callee_arg));
                }
                // set dfval-out of caller callinst
                PointToInfo tmp_dfval = (*result)[RI].first;
                PointToInfo &caller_dfval_out = (*result)[CI].second;
                PointToInfo old_caller_dfval_out = caller_dfval_out;
                // function return value can be pointer type
                if (RI->getReturnValue() &&
                    RI->getReturnValue()->getType()->isPointerTy())
                {
                    ValueSet v_set = tmp_dfval.pt_map[RI->getReturnValue()];
                    tmp_dfval.pt_map.erase(RI->getReturnValue());
                    tmp_dfval.pt_map[CI].insert(v_set.begin(), v_set.end());
                }
                // replace callee arg with caller arg in pt_map and field_pt_map
                for (auto pi = tmp_dfval.pt_map.begin(), pe = tmp_dfval.pt_map.end();
                     pi != pe; ++pi)
                {
                    for (auto ai = arg_map.begin(), ae = arg_map.end(); ai != ae; ++ai)
                    {
                        if (pi->second.count(ai->second))
                        {
                            pi->second.erase(ai->second);
                            pi->second.insert(ai->first);
                        }
                    }
                }
                for (auto pi = tmp_dfval.field_pt_map.begin(),
                          pe = tmp_dfval.field_pt_map.end();
                     pi != pe; ++pi)
                {
                    for (auto ai = arg_map.begin(), ae = arg_map.end(); ai != ae; ++ai)
                    {
                        if (pi->second.count(ai->second))
                        {
                            pi->second.erase(ai->second);
                            pi->second.insert(ai->first);
                        }
                    }
                }
                for (auto ai = arg_map.begin(), ae = arg_map.end(); ai != ae; ++ai)
                {
                    if (tmp_dfval.pt_map.count(ai->second))
                    {
                        ValueSet v_set = tmp_dfval.pt_map[ai->second];
                        tmp_dfval.pt_map.erase(ai->second);
                        tmp_dfval.pt_map[ai->first].insert(v_set.begin(), v_set.end());
                    }
                    if (tmp_dfval.field_pt_map.count(ai->second))
                    {
                        ValueSet v_set = tmp_dfval.field_pt_map[ai->second];
                        tmp_dfval.field_pt_map.erase(ai->second);
                        tmp_dfval.field_pt_map[ai->first].insert(v_set.begin(),
                                                                 v_set.end());
                    }
                }

                merge(&caller_dfval_out, tmp_dfval);
                if (caller_dfval_out != old_caller_dfval_out)
                {
                    worklist_.insert(caller);
                    // errs() << "[+]" << caller->getName()
                    //        << " is insert to worklist, because of callee\n";
                }
            }
        }
    }

    void transferOnStoreInst(StoreInst *SI, DataflowResult<PointToInfo>::Type *result)
    {
        PointToInfo dfval = (*result)[SI].first;
        ValueSet pts_to_insert;
        if (!dfval.pt_map[SI->getValueOperand()].empty())
        {
            ValueSet &pts = dfval.pt_map[SI->getValueOperand()];
            pts_to_insert.insert(pts.begin(), pts.end());
        }
        else
        {
            pts_to_insert.insert(SI->getValueOperand());
        }

        if (auto *GEP = dyn_cast<GetElementPtrInst>(SI->getPointerOperand()))
        {
            if (!dfval.pt_map[GEP->getPointerOperand()].empty())
            {
                ValueSet &pts = dfval.pt_map[GEP->getPointerOperand()];
                for (auto i = pts.begin(), e = pts.end(); i != e; ++i)
                {
                    Value *v = *i;
                    dfval.field_pt_map[v].clear();
                    dfval.field_pt_map[v].insert(pts_to_insert.begin(),
                                                 pts_to_insert.end());
                }
            }
            else
            {
                dfval.field_pt_map[GEP->getPointerOperand()].clear();
                dfval.field_pt_map[GEP->getPointerOperand()].insert(pts_to_insert.begin(),
                                                                    pts_to_insert.end());
            }
        }
        else
        {
            dfval.pt_map[SI->getPointerOperand()].clear();
            dfval.pt_map[SI->getPointerOperand()].insert(pts_to_insert.begin(),
                                                         pts_to_insert.end());
        }
        (*result)[SI].second = dfval;
    }

    void transferOnLoadInst(LoadInst *LI, DataflowResult<PointToInfo>::Type *result)
    {
        PointToInfo dfval = (*result)[LI].first;
        dfval.pt_map[LI].clear();
        if (auto *GEP = dyn_cast<GetElementPtrInst>(LI->getPointerOperand()))
        {
            if (!dfval.pt_map[GEP->getPointerOperand()].empty())
            {
                ValueSet &pts1 = dfval.pt_map[GEP->getPointerOperand()];
                for (auto i = pts1.begin(), e = pts1.end(); i != e; ++i)
                {
                    Value *v = *i;
                    ValueSet &pts2 = dfval.field_pt_map[v];
                    dfval.pt_map[LI].insert(pts2.begin(), pts2.end());
                }
            }
            else
            {
                ValueSet &pts = dfval.field_pt_map[GEP->getPointerOperand()];
                dfval.pt_map[LI].insert(pts.begin(), pts.end());
            }
        }
        else
        {
            ValueSet &pts = dfval.pt_map[LI->getPointerOperand()];
            dfval.pt_map[LI].insert(pts.begin(), pts.end());
        }
        (*result)[LI].second = dfval;
    }

    void transferOnGetElementPtrInst(GetElementPtrInst *GEP,
                                     DataflowResult<PointToInfo>::Type *result)
    {
        PointToInfo dfval = (*result)[GEP].first;
        dfval.pt_map[GEP].clear();
        // TODO
        if (!dfval.pt_map[GEP->getPointerOperand()].empty())
        {
            dfval.pt_map[GEP].insert(dfval.pt_map[GEP->getPointerOperand()].begin(),
                                     dfval.pt_map[GEP->getPointerOperand()].end());
        }
        else
        {
            dfval.pt_map[GEP].insert(GEP->getPointerOperand());
        }
        (*result)[GEP].second = dfval;
    }

    void transferOnBitCastInst(BitCastInst *BCI,
                               DataflowResult<PointToInfo>::Type *result)
    {
        PointToInfo dfval = (*result)[BCI].first;
        (*result)[BCI].second = dfval;
    }

    void transferOnMemCpyInst(MemCpyInst *MCI, DataflowResult<PointToInfo>::Type *result)
    {
        PointToInfo dfval = (*result)[MCI].first;

        auto *BCI0 = dyn_cast<BitCastInst>(MCI->getArgOperand(0));
        auto *BCI1 = dyn_cast<BitCastInst>(MCI->getArgOperand(1));
        if (!BCI0 || !BCI1)
        {
            (*result)[MCI].second = dfval;
            return;
        }
        Value *dst = BCI0->getOperand(0);
        Value *src = BCI1->getOperand(0);
        ValueSet &src_pts = dfval.pt_map[src];
        ValueSet &src_field_pts = dfval.field_pt_map[src];
        dfval.pt_map[dst].clear();
        dfval.pt_map[dst].insert(src_pts.begin(), src_pts.end());
        dfval.field_pt_map[dst].clear();
        dfval.field_pt_map[dst].insert(src_field_pts.begin(), src_field_pts.end());
        (*result)[MCI].second = dfval;
    }
};

class PointToPass : public ModulePass
{
public:
    static char ID;

private:
    DataflowResult<PointToInfo>::Type result_;
    FunctionSet worklist_;

public:
    PointToPass() : ModulePass(ID), result_(), worklist_() {}

    bool runOnModule(Module &M) override
    {
        PointToVisitor visitor;

        for (auto &F : M)
        {
            if (F.isIntrinsic())
                continue;
            worklist_.insert(&F);
        }

        while (!worklist_.empty())
        {
            Function *F = *(worklist_.begin());
            worklist_.erase(worklist_.begin());
            // errs() << "-----" << F->getName() << "-----\n";
            PointToInfo initval;
            compForwardDataflow(F, &visitor, &result_, initval);
            worklist_.insert(visitor.worklist_.begin(), visitor.worklist_.end());
            visitor.worklist_.clear();
        }
        dumpCallGraph(visitor.call_graph_);
        return false;
    }

private:
    void dumpCallGraph(const std::map<CallInst *, FunctionSet, CallInstCmp> &call_graph)
    {
        for (auto i = call_graph.begin(), e = call_graph.end(); i != e; ++i)
        {
            errs() << i->first->getDebugLoc().getLine() << " : ";
            for (auto ii = i->second.begin(), ee = i->second.end(); ii != ee; ++ii)
            {
                if (ii != i->second.begin())
                {
                    errs() << ", ";
                }

                errs() << (*ii)->getName();
            }
            errs() << "\n";
        }
    }
};

char PointToPass::ID = 0;
