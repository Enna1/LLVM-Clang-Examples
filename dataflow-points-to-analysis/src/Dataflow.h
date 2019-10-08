/************************************************************************
 *
 * @file Dataflow.h
 *
 * General dataflow framework
 *
 ***********************************************************************/

#ifndef _DATAFLOW_H_
#define _DATAFLOW_H_

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>  //inst_iterator
#include <llvm/Support/raw_ostream.h>
#include <map>

using namespace llvm;

///
/// Dummy class to provide a typedef for the detailed result set
/// For each basicblock, we compute its input dataflow val and its output
/// dataflow val
///
template <class T>
struct DataflowResult
{
    // <Instruction, <in, out>>
    typedef typename std::map<Instruction *, std::pair<T, T> > Type;
};

/// Base dataflow visitor class, defines the dataflow function
template <class T>
class DataflowVisitor
{
public:
    virtual ~DataflowVisitor() {}

    /// Dataflow Function invoked for each basic block
    ///
    /// @block the Basic Block
    /// @dfval the input dataflow value
    /// @isforward true to compute dfval forward, otherwise backward
    virtual void compDFVal(BasicBlock *block, typename DataflowResult<T>::Type *result,
                           bool isforward)
    {
        if (isforward == true)
        {
            for (BasicBlock::iterator ii = block->begin(), ie = block->end(); ii != ie;
                 ++ii)
            {
                Instruction *inst = &*ii;
                compDFVal(inst, result);
                if (Instruction *next_inst = inst->getNextNode())
                {
                    (*result)[next_inst].first = (*result)[inst].second;
                }
            }
        }
        else
        {
            return;
        }
    }

    ///
    /// Dataflow Function invoked for each instruction
    ///
    /// @inst the Instruction
    /// @dfval the input dataflow value
    /// @return true if dfval changed
    virtual void compDFVal(Instruction *inst,
                           typename DataflowResult<T>::Type *result) = 0;

    ///
    /// Merge of two dfvals, dest will be ther merged result
    /// @return true if dest changed
    ///
    virtual void merge(T *dest, const T &src) = 0;
};

///
/// Compute a forward iterated fixedpoint dataflow function, using a
/// user-supplied visitor function. Note that the caller must ensure that the
/// function is in fact a monotone function, as otherwise the fixedpoint may not
/// terminate.
///
/// @param fn The function
/// @param visitor A function to compute dataflow vals
/// @param result The results of the dataflow
/// @param initval The Initial dataflow value
template <class T>
void compForwardDataflow(Function *fn, DataflowVisitor<T> *visitor,
                         typename DataflowResult<T>::Type *result, const T &initval)
{
    std::set<BasicBlock *> worklist;

    // Initialize the worklist with all exit blocks
    for (Function::iterator bi = fn->begin(); bi != fn->end(); ++bi)
    {
        BasicBlock *bb = &*bi;
        worklist.insert(bb);
        for (auto ii = bb->begin(), ie = bb->end(); ii != ie; ++ii)
        {
            Instruction *I = &*ii;
            result->insert(std::make_pair(I, std::make_pair(initval, initval)));
        }
    }

    // Iteratively compute the dataflow result
    while (!worklist.empty())
    {
        BasicBlock *bb = *worklist.begin();
        worklist.erase(worklist.begin());

        // Merge all incoming value
        Instruction *bb_first_inst = &*(bb->begin());
        Instruction *bb_last_inst = &*(--bb->end());

        T bbinval = (*result)[bb_first_inst].first;
        for (auto pi = pred_begin(bb), pe = pred_end(bb); pi != pe; pi++)
        {
            BasicBlock *pred = *pi;
            Instruction *pred_last_inst = &*(--pred->end());
            visitor->merge(&bbinval, (*result)[pred_last_inst].second);
        }

        (*result)[bb_first_inst].first = bbinval;
        T old_bboutval = (*result)[bb_last_inst].second;
        visitor->compDFVal(bb, result, true);

        // If outgoing value changed, propagate it along the CFG
        if (old_bboutval == (*result)[bb_last_inst].second)
            continue;

        for (auto si = succ_begin(bb), se = succ_end(bb); si != se; si++)
        {
            worklist.insert(*si);
        }
    }
}

///
/// Compute a backward iterated fixedpoint dataflow function, using a
/// user-supplied visitor function. Note that the caller must ensure that the
/// function is in fact a monotone function, as otherwise the fixedpoint may not
/// terminate.
///
/// @param fn The function
/// @param visitor A function to compute dataflow vals
/// @param result The results of the dataflow
/// @param initval The Initial dataflow value
template <class T>
void compBackwardDataflow(Function *fn, DataflowVisitor<T> *visitor,
                          typename DataflowResult<T>::Type *result, const T &initval)
{
    return;
}

template <class T>
void printDataflowResult(raw_ostream &out,
                         const typename DataflowResult<T>::Type &dfresult)
{
    for (typename DataflowResult<T>::Type::const_iterator it = dfresult.begin();
         it != dfresult.end(); ++it)
    {
        if (it->first == NULL)
            out << "*";
        else
            it->first->dump();
        out << "\n\tin : " << it->second.first << "\n\tout :  " << it->second.second
            << "\n";
    }
}

template <class T>
void printDataflowResult(raw_ostream &out,
                         const typename DataflowResult<T>::Type &dfresult, Function *F)
{
    for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i)
    {
        Instruction *I = &(*i);
        if (!dfresult.count(I))
        {
            out << "*";
        }
        else
        {
            I->dump();
            out << "\n\tin : " << dfresult.at(I).first
                << "\n\tout :  " << dfresult.at(I).second << "\n";
        }
    }
}
#endif /* !_DATAFLOW_H_ */
