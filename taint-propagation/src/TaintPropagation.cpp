//===- TaintPropagation.cpp - Propagate taint values -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
//===----------------------------------------------------------------------===//
//
// This file implements a transform pass that attaches !taint metadata
// to instructions. For a given instruction, the metadata, if present, indicates
// the set of tainted operands before this instruction executed.
//
//===----------------------------------------------------------------------===//

#include "TaintPropagation.h"
#include "llvm/Analysis/IteratedDominanceFrontier.h"
#include "llvm/Analysis/SparsePropagation.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/IPO.h"
#include <unordered_map>
using namespace llvm;

#define DEBUG_TYPE "taint-propagation"

constexpr char MD_TAINT[] = "taint";

/// The maximum number of instructions to track per lattice value. Once the number exceeds
/// this threshold, the lattice value becomes overdefined.
static cl::opt<unsigned> MaxInstructionsPerValue(
    "max-instructions-per-value", cl::Hidden, cl::init(128),
    cl::desc("The maximum number of instructions to track per lattice value"));

/// Cache that map Function to its DominatorTree
static std::unordered_map<Function *, DominatorTree> FnToDTMap;

/// Return if any one of DefInsts can reach UseInst
/// A DefInst can reach a UseInst only if the UseInst is dominated by the DefInst or one
/// of its iterated dominance frontiers
/// NOTE:
/// - Specail case: if DefInsts is empty, we consider DefInsts can reach UseInst
/// - UseInst should not occur in DefInsts
static bool reachable(const std::vector<Instruction *> &DefInsts, Instruction *UseInst)
{
    if (DefInsts.empty())
        return true;

    Function *F = UseInst->getFunction();
    if (FnToDTMap.find(F) == FnToDTMap.end())
    {
        FnToDTMap.emplace(std::make_pair(F, DominatorTree(*F)));
    }

    DominatorTree &DT = FnToDTMap[F];
    for (auto I : DefInsts)
    {
        // An instruction doesn't dominate a use in itself.
        if (DT.dominates(I, UseInst))
            return true;
    }

    ForwardIDFCalculator IDF(DT);
    SmallPtrSet<BasicBlock *, 32> Blocks;
    for (auto I : DefInsts)
    {
        // If this DefInst is exactly the UseInst, skip
        if (I == UseInst)
            continue;
        Blocks.insert(I->getParent());
    }
    IDF.setDefiningBlocks(Blocks);
    SmallVector<BasicBlock *, 32> IDFBlocks;
    IDF.calculate(IDFBlocks);
    for (auto *BB : IDFBlocks)
    {
        if (DT.dominates(BB, UseInst->getParent()))
            return true;
    }

    return false;
}

/// To enable interprocedural analysis, we assign LLVM values to the following
/// groups. The register group represents SSA registers, the memory group represents
/// in-memory values, and the return group represents the return values of functions.
/// An LLVM Value can technically be in more than one group.
enum class IPOGrouping : int8_t
{
    Register = 0,
    Memory,
    Return,
};

/// Our lattice keys are PointerIntPairs composed of LLVM values and groupings.
using TaintLatticeKey = PointerIntPair<Value *, 2, IPOGrouping>;

/// The lattice value type used by our custom lattice function. It holds the
/// lattice state, and a set of instructions.
class TaintLatticeVal
{
public:
    /// The states of the lattice values. Only the Tainted state is interesting.
    enum TaintLatticeStateTy
    {
        Undefined,
        Tainted,
        Overdefined,
        Untracked
    };

    /// Comparator for sorting the instructions set. We want to keep the order
    /// deterministic for testing, etc.
    struct Compare
    {
        bool operator()(const Value *LHS, const Value *RHS) const
        {
            return LHS < RHS;
        }
    };

    TaintLatticeVal() : LatticeState(Undefined) {}
    TaintLatticeVal(TaintLatticeStateTy LatticeState) : LatticeState(LatticeState) {}
    TaintLatticeVal(std::vector<Instruction *> &&TaintedAtInsts)
        : LatticeState(Tainted), TaintedAtInsts(std::move(TaintedAtInsts))
    {
        assert(std::is_sorted(this->TaintedAtInsts.begin(), this->TaintedAtInsts.end(),
                              Compare()));
    }

    /// Get a reference to the instructions set held by this lattice value.
    /// For states other than Tainted, the number of instructions is zero.
    /// For states Tainted, the number of instructions is non-zero other than the
    /// corresponding lattice key refers to arguments or return values of a function.
    const std::vector<Instruction *> &getTaintedAtInsts() const
    {
        return TaintedAtInsts;
    }

    /// Returns true if the lattice value is in the Tainted state.
    bool isTainted() const
    {
        return LatticeState == Tainted;
    }

    /// Just set the lattic value to Tainted state, there is no change to the instructions
    /// set `TaintedAtInsts`
    void setTainted()
    {
        LatticeState = Tainted;
    }

    bool operator==(const TaintLatticeVal &RHS) const
    {
        return LatticeState == RHS.LatticeState && TaintedAtInsts == RHS.TaintedAtInsts;
    }

    bool operator!=(const TaintLatticeVal &RHS) const
    {
        return LatticeState != RHS.LatticeState || TaintedAtInsts != RHS.TaintedAtInsts;
    }

private:
    /// Holds the state this lattice value is in.
    TaintLatticeStateTy LatticeState;

    /// This set is empty for lattice values in the undefined, overdefined,
    /// and untracked states. The maximum size of this set is controlled by
    /// MaxInstructionsPerValue.
    std::vector<Instruction *> TaintedAtInsts;
};

class TaintSolver;

/// The custom lattice function used by the TaintSolver.
/// It handles merging lattice values and computing new lattice values.
/// It also computes the lattice values that change as a result of executing instructions.
class TaintLatticeFunc
{
public:
    TaintLatticeFunc() {}
    ~TaintLatticeFunc() {}

    TaintLatticeVal getUndefVal() const
    {
        return TaintLatticeVal::Undefined;
    }

    TaintLatticeVal getOverdefinedVal() const
    {
        return TaintLatticeVal::Overdefined;
    }

    TaintLatticeVal getUntrackedVal() const
    {
        return TaintLatticeVal::Untracked;
    }

    bool IsUntrackedValue(TaintLatticeKey Key);

    /// ComputeLatticeVal - Compute and return a TaintLatticeVal corresponding to the
    /// given TaintLatticeKey.
    TaintLatticeVal ComputeLatticeVal(TaintLatticeKey Key);

    /// IsSpecialCasedPHI - Given a PHI node, determine whether this PHI node is
    /// one that the we want to handle through ComputeInstructionState.
    /// Actually this function always returns true.
    bool IsSpecialCasedPHI(PHINode *PN);

    /// Merge the two given lattice values. The interesting cases are merging two
    /// instructions set and a instructions set with an Undefined value. For
    /// these cases, we simply union the instruction sets. If the size of the union
    /// is greater than the maximum instructions we track, the merged value is
    /// overdefined.
    TaintLatticeVal MergeValues(TaintLatticeVal X, TaintLatticeVal Y);

    /// Compute the lattice values that change as a result of executing the given
    /// instruction. The changed values are stored in `ChangedValues`.
    void ComputeInstructionState(
        Instruction &I, DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
        TaintSolver &TS);

    /// Print the given TaintLatticeVal to the specified stream.
    void PrintLatticeVal(TaintLatticeVal LV, raw_ostream &OS);

    /// Print the given TaintLatticeKey to the specified stream.
    void PrintLatticeKey(TaintLatticeKey Key, raw_ostream &OS);

    /// GetValueFromLatticeVal - If the given LatticeVal is representable as an
    /// LLVM value, return it; otherwise, return nullptr. If a type is given, the
    /// returned value must have the same type. This function is used by the
    /// generic solver in attempting to resolve branch and switch conditions.
    Value *GetValueFromLatticeVal(TaintLatticeVal LV, Type *Ty = nullptr);

private:
    /// Handle PHINode. The PHINode state is the merge of the incoming values states
    void visitPHINode(PHINode &I,
                      DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
                      TaintSolver &TS);

    /// Handle GetElementPtrInst. The GetElementPtrInst state is the merge of the
    /// GetElementPtrInst state with the pointer operand state.
    void visitGetElementPtr(GetElementPtrInst &I,
                            DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
                            TaintSolver &TS);

    /// Handle LoadInst. The loaded value state is the merge of the loaded value state
    /// with the pointer operand state.
    void visitLoad(LoadInst &I, DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
                   TaintSolver &TS);

    /// Handle StoreInst. If the stored value is Tainted, we set the state of the pointer
    /// operand to Tainted and track the values that the pointer operand depends on.
    /// We also set the state of the values that the pointer operand depends on to
    /// Tainted and handle the merge of pointer type arguments in this function.
    void visitStore(StoreInst &I,
                    DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
                    TaintSolver &TS);

    /// Handle MemTransferInst. A MemTransferInstMem is MemCpyInst or MemMoveInst.
    /// The destination value state is the merge of the destination value state
    /// with the source operand state.
    void visitMemTransfer(MemTransferInst &I,
                          DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
                          TaintSolver &TS);

    /// Handle CallSite. The state of a called function's formal arguments is
    /// the merge of the argument state with the call sites corresponding actual
    /// argument state. The call site state is the merge of the call site state
    /// with the returned value state of the called function.
    void visitCallSite(CallSite CS,
                       DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
                       TaintSolver &TS);

    /// Handle ReturnInst. The function's return state is the merge of
    /// the returned value state and the function's return state.
    void visitReturn(ReturnInst &I,
                     DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
                     TaintSolver &TS);

    /// Handle SelectInst. The select instruction state is the merge the
    /// true and false value states.
    void visitSelect(SelectInst &I,
                     DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
                     TaintSolver &TS);

    // Handle CastInst.
    void visitCast(CastInst &I, DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
                   TaintSolver &TS);

    /// Handle all other instructions.
    void visitInst(Instruction &I,
                   DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
                   TaintSolver &TS);

    ///
    void updateDependencyValueState(
        Value *V, TaintLatticeVal TLV,
        DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues, TaintSolver &TS);
};

/// TaintSolver - This class is slight modified version of llvm::SparseSolver
class TaintSolver
{
    /// LatticeFunc - This is the object that knows the lattice and how to
    /// compute transfer functions.
    TaintLatticeFunc *LatticeFunc;

    /// ValueState - Holds the TaintLatticeVals associated with TaintLatticeKeys.
    DenseMap<TaintLatticeKey, TaintLatticeVal> ValueState;

    /// BBExecutable - Holds the basic blocks that are executable.
    SmallPtrSet<BasicBlock *, 16> BBExecutable;

    /// ValueWorkList - Holds values that should be processed.
    SmallVector<Value *, 64> ValueWorkList;

    /// BBWorkList - Holds basic blocks that should be processed.
    SmallVector<BasicBlock *, 64> BBWorkList;

    using Edge = std::pair<BasicBlock *, BasicBlock *>;

    /// KnownFeasibleEdges - Entries in this set are edges which have already had
    /// PHI nodes retriggered.
    std::set<Edge> KnownFeasibleEdges;

    /// ValueDependencyMap - Map a value to a set of values that the value depends on.
    const std::unordered_map<Value *, SmallPtrSet<Value *, 16>> &ValueDependencyMap;

public:
    explicit TaintSolver(
        TaintLatticeFunc *Lattice,
        const std::unordered_map<Value *, SmallPtrSet<Value *, 16>> &ValueDependencyMap)
        : LatticeFunc(Lattice), ValueDependencyMap(ValueDependencyMap)
    {
    }

    TaintSolver(const TaintSolver &) = delete;
    TaintSolver &operator=(const TaintSolver &) = delete;

    void Solve();

    void Print(raw_ostream &OS) const;

    /// getExistingValueState - Return the TaintLatticeVal object corresponding to the
    /// given value from the ValueState map. If the value is not in the map,
    /// UntrackedVal is returned, unlike the getValueState method.
    TaintLatticeVal getExistingValueState(TaintLatticeKey Key) const;

    /// getValueState - Return the TaintLatticeVal object corresponding to the given
    /// value from the ValueState map. If the value is not in the map, its state
    /// is initialized.
    TaintLatticeVal getValueState(TaintLatticeKey Key);

    /// hasDependency - Return true if the given value is in ValueDependencyMap.
    bool hasDependency(Value *V);

    /// getDependency - Return a set of values that the given value depends on according
    /// to ValueDependencyMap.
    SmallPtrSet<Value *, 16> getDependency(Value *V);

    /// isEdgeFeasible - Return true if the control flow edge from the 'From'
    /// basic block to the 'To' basic block is currently feasible.  If
    /// AggressiveUndef is true, then this treats values with unknown lattice
    /// values as undefined.  This is generally only useful when solving the
    /// lattice, not when querying it.
    bool isEdgeFeasible(BasicBlock *From, BasicBlock *To, bool AggressiveUndef = false);

    /// isBlockExecutable - Return true if there are any known feasible
    /// edges into the basic block.  This is generally only useful when
    /// querying the lattice.
    bool isBlockExecutable(BasicBlock *BB) const
    {
        return BBExecutable.count(BB);
    }

    /// MarkBlockExecutable - This method can be used by clients to mark all of
    /// the blocks that are known to be intrinsically live in the processed unit.
    void MarkBlockExecutable(BasicBlock *BB);

private:
    /// UpdateState - When the state of some TaintLatticeKey is potentially updated to
    /// the given TaintLatticeVal, this function notices and adds the LLVM value
    /// corresponding the key to the work list, if needed.
    void UpdateState(TaintLatticeKey Key, TaintLatticeVal LV);

    /// markEdgeExecutable - Mark a basic block as executable, adding it to the BB
    /// work list if it is not already executable.
    void markEdgeExecutable(BasicBlock *Source, BasicBlock *Dest);

#if LLVM_VERSION_MAJOR >= 8
    void getFeasibleSuccessors(Instruction &TI, SmallVectorImpl<bool> &Succs,
                               bool AggressiveUndef);
    void visitTerminator(Instruction &TI);
#else
    /// getFeasibleSuccessors - Return a vector of booleans to indicate which
    /// successors are reachable from a given terminator instruction.
    void getFeasibleSuccessors(TerminatorInst &TI, SmallVectorImpl<bool> &Succs,
                               bool AggressiveUndef);
    void visitTerminator(TerminatorInst &TI);
#endif

    void visitInst(Instruction &I);
    void visitPHINode(PHINode &I);

    Value *getValueFromLatticeKey(TaintLatticeKey Key)
    {
        return Key.getPointer();
    }

    TaintLatticeKey getLatticeKeyFromValue(Value *V)
    {
        return TaintLatticeKey(V, IPOGrouping::Register);
    }
};

//===----------------------------------------------------------------------===//
//                          TaintLatticeFunc Implementation
//===----------------------------------------------------------------------===//

bool TaintLatticeFunc::IsUntrackedValue(TaintLatticeKey Key)
{
    return false;
}

TaintLatticeVal TaintLatticeFunc::ComputeLatticeVal(TaintLatticeKey Key)
{
    return getUndefVal();
}

bool TaintLatticeFunc::IsSpecialCasedPHI(PHINode *PN)
{
    return true;
}

TaintLatticeVal TaintLatticeFunc::MergeValues(TaintLatticeVal X, TaintLatticeVal Y)
{
    if (X == getOverdefinedVal() || Y == getOverdefinedVal())
        return getOverdefinedVal();
    if (X == getUndefVal() && Y == getUndefVal())
        return getUndefVal();
    std::vector<Instruction *> Union;
    std::set_union(X.getTaintedAtInsts().begin(), X.getTaintedAtInsts().end(),
                   Y.getTaintedAtInsts().begin(), Y.getTaintedAtInsts().end(),
                   std::back_inserter(Union), TaintLatticeVal::Compare{});
    if (Union.size() > MaxInstructionsPerValue)
        return getOverdefinedVal();
    return TaintLatticeVal(std::move(Union));
}

void TaintLatticeFunc::ComputeInstructionState(
    Instruction &I, DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
    TaintSolver &TS)
{
    switch (I.getOpcode())
    {
    case Instruction::PHI:
        return visitPHINode(*cast<PHINode>(&I), ChangedValues, TS);
    case Instruction::GetElementPtr:
        return visitGetElementPtr(*cast<GetElementPtrInst>(&I), ChangedValues, TS);
    case Instruction::Load:
        return visitLoad(*cast<LoadInst>(&I), ChangedValues, TS);
    case Instruction::Store:
        return visitStore(*cast<StoreInst>(&I), ChangedValues, TS);
    case Instruction::Call:
    {
        if (auto *MTI = dyn_cast<MemTransferInst>(&I))
        {
            return visitMemTransfer(*MTI, ChangedValues, TS);
        }
        else
        {
            return visitCallSite(cast<CallInst>(&I), ChangedValues, TS);
        }
    }
    case Instruction::Invoke:
        return visitCallSite(cast<InvokeInst>(&I), ChangedValues, TS);
    case Instruction::Ret:
        return visitReturn(*cast<ReturnInst>(&I), ChangedValues, TS);
    case Instruction::Select:
        return visitSelect(*cast<SelectInst>(&I), ChangedValues, TS);
    case Instruction::Trunc:
    case Instruction::ZExt:
    case Instruction::SExt:
    case Instruction::FPTrunc:
    case Instruction::FPExt:
    case Instruction::UIToFP:
    case Instruction::SIToFP:
    case Instruction::FPToUI:
    case Instruction::FPToSI:
    case Instruction::PtrToInt:
    case Instruction::IntToPtr:
    case Instruction::BitCast:
    case Instruction::AddrSpaceCast:
        return visitCast(*cast<CastInst>(&I), ChangedValues, TS);
    default:
        return visitInst(I, ChangedValues, TS);
    }
}

void TaintLatticeFunc::PrintLatticeVal(TaintLatticeVal LV, raw_ostream &OS)
{
    if (LV == getUndefVal())
        OS << "Undefined";
    else if (LV == getOverdefinedVal())
        OS << "Overdefined";
    else if (LV == getUntrackedVal())
        OS << "Untracked";
    else
        OS << "Tainted";
}

void TaintLatticeFunc::PrintLatticeKey(TaintLatticeKey Key, raw_ostream &OS)
{
    if (Key.getInt() == IPOGrouping::Register)
        OS << "<reg> ";
    else if (Key.getInt() == IPOGrouping::Memory)
        OS << "<mem> ";
    else if (Key.getInt() == IPOGrouping::Return)
        OS << "<ret> ";
    else
        OS << *Key.getPointer();
}

Value *TaintLatticeFunc::GetValueFromLatticeVal(TaintLatticeVal LV, Type *Ty)
{
    return nullptr;
}

void TaintLatticeFunc::visitPHINode(
    PHINode &I, DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
    TaintSolver &TS)
{
    auto RegPhi = TaintLatticeKey(&I, IPOGrouping::Register);
    for (unsigned i = 0, e = I.getNumIncomingValues(); i != e; ++i)
    {
        auto RegOp = TaintLatticeKey(I.getIncomingValue(i), IPOGrouping::Register);
        if (TS.getValueState(RegOp).isTainted() &&
            reachable(TS.getValueState(RegOp).getTaintedAtInsts(), &I))
        {
            ChangedValues[RegPhi] =
                MergeValues(TS.getValueState(RegPhi), TaintLatticeVal({ &I }));
            break;
        }
    }
}

void TaintLatticeFunc::visitGetElementPtr(
    GetElementPtrInst &I, DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
    TaintSolver &TS)
{
    auto RegI = TaintLatticeKey(&I, IPOGrouping::Register);
    auto RegP = TaintLatticeKey(I.getPointerOperand(), IPOGrouping::Register);
    if (TS.getValueState(RegP).isTainted() &&
        reachable(TS.getValueState(RegP).getTaintedAtInsts(), &I))
    {
        ChangedValues[RegI] =
            MergeValues(TS.getValueState(RegI), TaintLatticeVal({ &I }));
    }
}

void TaintLatticeFunc::visitLoad(
    LoadInst &I, DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
    TaintSolver &TS)
{
    auto RegI = TaintLatticeKey(&I, IPOGrouping::Register);
    auto RegP = TaintLatticeKey(I.getPointerOperand(), IPOGrouping::Register);
    if (TS.getValueState(RegP).isTainted() &&
        reachable(TS.getValueState(RegP).getTaintedAtInsts(), &I))
    {
        ChangedValues[RegI] =
            MergeValues(TS.getValueState(RegI), TaintLatticeVal({ &I }));
    }
}

void TaintLatticeFunc::visitStore(
    StoreInst &I, DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
    TaintSolver &TS)
{
    auto RegV = TaintLatticeKey(I.getValueOperand(), IPOGrouping::Register);
    auto RegP = TaintLatticeKey(I.getPointerOperand(), IPOGrouping::Register);
    if (TS.getValueState(RegV).isTainted() &&
        reachable(TS.getValueState(RegV).getTaintedAtInsts(), &I))
    {
        // Update the state of the pointer operand
        ChangedValues[RegP] =
            MergeValues(TS.getValueState(RegP), TaintLatticeVal({ &I }));

        // Update the state of the set of values that the pointer operand depends on
        updateDependencyValueState(I.getPointerOperand(), TaintLatticeVal({ &I }),
                                   ChangedValues, TS);
    }

    // If the type of some function arguments is pointer, inform the solver that the
    // caller function is executable, and perform merges for these pointer arguments.
    // NOTE: This part of code *must* be placed in `visitStore`
    // FIXME: Is there any way to avoid perform merges for function pointer arguments when
    // the pointer operand of StoreInst and the set of values that the pointer operand
    // depends are unrelated to function pointer arguments?
    SmallVector<Argument *, 4> AffectedFnPointerArguments;
    if (auto *Arg = dyn_cast<Argument>(I.getPointerOperand()))
    {
        AffectedFnPointerArguments.push_back(Arg);
    }
    if (TS.hasDependency(I.getPointerOperand()))
    {
        SmallPtrSet<Value *, 16> Values = TS.getDependency(I.getPointerOperand());
        for (Value *V : Values)
        {
            if (!V->getType()->isPointerTy())
                continue;
            if (auto *Arg = dyn_cast<Argument>(V))
            {
                AffectedFnPointerArguments.push_back(Arg);
            }
        }
    }
    Function *F = I.getFunction();
    for (Argument *Arg : AffectedFnPointerArguments)
    {
        for (User *U : F->users())
        {
            if (auto CS = CallSite(U))
            {
                TS.MarkBlockExecutable(CS.getInstruction()->getParent());
                auto ArgActual = TaintLatticeKey(CS.getArgument(Arg->getArgNo()),
                                                 IPOGrouping::Register);
                auto ArgFormal = TaintLatticeKey(Arg, IPOGrouping::Register);
                if (TS.getValueState(ArgFormal).isTainted())
                {
                    ChangedValues[ArgActual] =
                        MergeValues(TS.getValueState(ArgActual),
                                    TaintLatticeVal({ CS.getInstruction() }));
                }
            }
        }
    }
}

void TaintLatticeFunc::visitMemTransfer(
    MemTransferInst &I, DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
    TaintSolver &TS)
{
    auto RegSrc = TaintLatticeKey(I.getOperand(1), IPOGrouping::Register);
    auto RegDst = TaintLatticeKey(I.getOperand(0), IPOGrouping::Register);
    if (TS.getValueState(RegSrc).isTainted() &&
        reachable(TS.getValueState(RegSrc).getTaintedAtInsts(), &I))
    {
        ChangedValues[RegDst] =
            MergeValues(TS.getValueState(RegDst), TaintLatticeVal({ &I }));

        // Update the state of the set of values that the pointer operand depends on
        updateDependencyValueState(I.getOperand(0), TaintLatticeVal({ &I }),
                                   ChangedValues, TS);
    }
}

void TaintLatticeFunc::visitCallSite(
    CallSite CS, DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
    TaintSolver &TS)
{
    Function *F = CS.getCalledFunction();
    Instruction *I = CS.getInstruction();
    // Initialize taint source
#define HANDLE_TAINT_SOURCE(FUNC_NAME, ARGS)                                             \
    do                                                                                   \
    {                                                                                    \
        if (F && F->getName().equals(FUNC_NAME))                                         \
        {                                                                                \
            std::vector<int8_t> Args(ARGS);                                              \
            for (const auto &i : Args)                                                   \
            {                                                                            \
                if (i == -1)                                                             \
                {                                                                        \
                    auto Reg = TaintLatticeKey(I, IPOGrouping::Register);                \
                    ChangedValues[Reg] =                                                 \
                        MergeValues(TS.getValueState(Reg), TaintLatticeVal({ I }));      \
                    updateDependencyValueState(I, TaintLatticeVal({ I }), ChangedValues, \
                                               TS);                                      \
                }                                                                        \
                else                                                                     \
                {                                                                        \
                    auto Reg = TaintLatticeKey(I->getOperand(i), IPOGrouping::Register); \
                    ChangedValues[Reg] =                                                 \
                        MergeValues(TS.getValueState(Reg), TaintLatticeVal({ I }));      \
                    updateDependencyValueState(I->getOperand(i), TaintLatticeVal({ I }), \
                                               ChangedValues, TS);                       \
                }                                                                        \
            }                                                                            \
            return;                                                                      \
        }                                                                                \
    } while (false)
    // Perform taint propagation on lib call
#define HANDLE_TAINT_PROPAGATION_LIBCALL(FUNC_NAME, SRC_ARGS, DST_ARGS)                  \
    do                                                                                   \
    {                                                                                    \
        if (F && F->getName().equals(FUNC_NAME))                                         \
        {                                                                                \
            std::vector<int8_t> SrcArgs(SRC_ARGS);                                       \
            std::vector<int8_t> DstArgs(DST_ARGS);                                       \
            bool SrcTainted = false;                                                     \
            for (const auto &i : SrcArgs)                                                \
            {                                                                            \
                auto Reg = TaintLatticeKey(I->getOperand(i), IPOGrouping::Register);     \
                if (TS.getValueState(Reg).isTainted() &&                                 \
                    reachable(TS.getValueState(Reg).getTaintedAtInsts(), I))             \
                {                                                                        \
                    SrcTainted = true;                                                   \
                    break;                                                               \
                }                                                                        \
            }                                                                            \
            if (SrcTainted)                                                              \
            {                                                                            \
                for (const auto &i : DstArgs)                                            \
                {                                                                        \
                    auto Reg = TaintLatticeKey(I->getOperand(i), IPOGrouping::Register); \
                    {                                                                    \
                        ChangedValues[Reg] =                                             \
                            MergeValues(TS.getValueState(Reg), TaintLatticeVal({ I }));  \
                        updateDependencyValueState(I->getOperand(i),                     \
                                                   TaintLatticeVal({ I }),               \
                                                   ChangedValues, TS);                   \
                    }                                                                    \
                }                                                                        \
            }                                                                            \
        }                                                                                \
    } while (false)
#include "Taint.def"
#undef HANDLE_TAINT_SOURCE
#undef HANDLE_TAINT_PROPAGATION_LIBCALL

    // If this is an indirect call or we can't track the function, there's nothing to do.
    if (!F || !F->hasExactDefinition())
    {
        return;
    }

    // Inform the solver that the called function is executable, and perform
    // merges for the arguments and return value.
    TS.MarkBlockExecutable(&F->front());
    for (Argument &Arg : F->args())
    {
        auto ArgFormal = TaintLatticeKey(&Arg, IPOGrouping::Register);
        auto ArgActual =
            TaintLatticeKey(CS.getArgument(Arg.getArgNo()), IPOGrouping::Register);
        if (TS.getValueState(ArgActual).isTainted() &&
            reachable(TS.getValueState(ArgActual).getTaintedAtInsts(), I))
        {
            // NOTE: call setTainted() to mark function arguments as tainted, and
            // `TaintedAtInsts` is empty
            ChangedValues[ArgFormal].setTainted();
        }
    }

    // Void return, No need to create and update lattice state as no one can
    // use it.
    if (I->getType()->isVoidTy())
        return;
    auto RetF = TaintLatticeKey(F, IPOGrouping::Return);
    auto RegI = TaintLatticeKey(I, IPOGrouping::Register);
    if (TS.getValueState(RetF).isTainted())
    {
        ChangedValues[RegI] = MergeValues(TS.getValueState(RegI), TaintLatticeVal({ I }));
    }
}

void TaintLatticeFunc::visitReturn(
    ReturnInst &I, DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
    TaintSolver &TS)
{
    Function *F = I.getFunction();
    if (F->getReturnType()->isVoidTy())
        return;
    auto RegI = TaintLatticeKey(I.getReturnValue(), IPOGrouping::Register);
    auto RetF = TaintLatticeKey(F, IPOGrouping::Return);
    if (TS.getValueState(RegI).isTainted() &&
        reachable(TS.getValueState(RegI).getTaintedAtInsts(), &I))
    {
        ChangedValues[RetF].setTainted();
    }
}

void TaintLatticeFunc::visitSelect(
    SelectInst &I, DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
    TaintSolver &TS)
{
    auto RegI = TaintLatticeKey(&I, IPOGrouping::Register);
    auto RegT = TaintLatticeKey(I.getTrueValue(), IPOGrouping::Register);
    auto RegF = TaintLatticeKey(I.getFalseValue(), IPOGrouping::Register);
    if ((TS.getValueState(RegT).isTainted() &&
         reachable(TS.getValueState(RegT).getTaintedAtInsts(), &I)) ||
        (TS.getValueState(RegF).isTainted() &&
         reachable(TS.getValueState(RegF).getTaintedAtInsts(), &I)))
    {
        ChangedValues[RegI] =
            MergeValues(TS.getValueState(RegI), TaintLatticeVal({ &I }));
    }
}

void TaintLatticeFunc::visitCast(
    CastInst &I, DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
    TaintSolver &TS)
{
    TaintLatticeKey Src = TaintLatticeKey(I.getOperand(0), IPOGrouping::Register);
    TaintLatticeKey Dst = TaintLatticeKey(&I, IPOGrouping::Register);
    if (TS.getValueState(Src).isTainted() &&
        reachable(TS.getValueState(Src).getTaintedAtInsts(), &I))
    {
        ChangedValues[Dst] = MergeValues(TS.getValueState(Dst), TaintLatticeVal({ &I }));
    }
}

void TaintLatticeFunc::visitInst(
    Instruction &I, DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues,
    TaintSolver &TS)
{
    auto RegI = TaintLatticeKey(&I, IPOGrouping::Register);
    ChangedValues[RegI] = TS.getValueState(RegI);
    for (Use &U : I.operands())
    {
        Value *V = U.get();
        auto RegV = TaintLatticeKey(V, IPOGrouping::Register);
        if (TS.getValueState(RegV).isTainted())
        {
            ChangedValues[RegI] =
                MergeValues(ChangedValues[RegI], TaintLatticeVal({ &I }));
            return;
        }
    }
}

void TaintLatticeFunc::updateDependencyValueState(
    Value *V, TaintLatticeVal TLV,
    DenseMap<TaintLatticeKey, TaintLatticeVal> &ChangedValues, TaintSolver &TS)
{
    if (TS.hasDependency(V))
    {
        SmallPtrSet<Value *, 16> Values = TS.getDependency(V);
        for (Value *EachValue : Values)
        {
            auto Reg = TaintLatticeKey(EachValue, IPOGrouping::Register);
            ChangedValues[Reg] = MergeValues(TS.getValueState(Reg), TLV);
        }
    }
}

//===----------------------------------------------------------------------===//
//                          TaintSolver Implementation
//===----------------------------------------------------------------------===//
TaintLatticeVal TaintSolver::getExistingValueState(TaintLatticeKey Key) const
{
    auto I = ValueState.find(Key);
    return I != ValueState.end() ? I->second : LatticeFunc->getUntrackedVal();
}

TaintLatticeVal TaintSolver::getValueState(TaintLatticeKey Key)
{
    auto I = ValueState.find(Key);
    if (I != ValueState.end())
        return I->second;  // Common case, in the map

    if (LatticeFunc->IsUntrackedValue(Key))
        return LatticeFunc->getUntrackedVal();
    TaintLatticeVal LV = LatticeFunc->ComputeLatticeVal(Key);

    // If this value is untracked, don't add it to the map.
    if (LV == LatticeFunc->getUntrackedVal())
        return LV;
    return ValueState[Key] = std::move(LV);
}

void TaintSolver::UpdateState(TaintLatticeKey Key, TaintLatticeVal LV)
{
    auto I = ValueState.find(Key);
    if (I != ValueState.end() && I->second == LV)
        return;  // No change.

    // Update the state of the given TaintLatticeKey and add its corresponding LLVM
    // value to the work list.
    ValueState[Key] = std::move(LV);
    if (Value *V = getValueFromLatticeKey(Key))
        ValueWorkList.push_back(V);
}

bool TaintSolver::hasDependency(Value *V)
{
    return ValueDependencyMap.count(V);
}

SmallPtrSet<Value *, 16> TaintSolver::getDependency(Value *V)
{
    SmallPtrSet<Value *, 16> Values(ValueDependencyMap.at(V));
    bool changed = true;
    while (changed)
    {
        changed = false;
        size_t ValuesSize = Values.size();
        SmallPtrSet<Value *, 16> Tmp;
        for (auto *Each_V : Values)
        {
            if (ValueDependencyMap.count(Each_V))
            {
                Tmp.insert(ValueDependencyMap.at(Each_V).begin(),
                           ValueDependencyMap.at(Each_V).end());
            }
        }
        Values.insert(Tmp.begin(), Tmp.end());
        if (ValuesSize != Values.size())
            changed = true;
    }
    return Values;
}

void TaintSolver::MarkBlockExecutable(BasicBlock *BB)
{
    if (!BBExecutable.insert(BB).second)
        return;
    BBWorkList.push_back(BB);  // Add the block to the work list!
}

void TaintSolver::markEdgeExecutable(BasicBlock *Source, BasicBlock *Dest)
{
    if (!KnownFeasibleEdges.insert(Edge(Source, Dest)).second)
        return;  // This edge is already known to be executable!

    if (BBExecutable.count(Dest))
    {
        // The destination is already executable, but we just made an edge
        // feasible that wasn't before.  Revisit the PHI nodes in the block
        // because they have potentially new operands.
        for (BasicBlock::iterator I = Dest->begin(); isa<PHINode>(I); ++I)
            visitPHINode(*cast<PHINode>(I));
    }
    else
    {
        MarkBlockExecutable(Dest);
    }
}

#if LLVM_VERSION_MAJOR >= 8
void TaintSolver::getFeasibleSuccessors(Instruction &TI, SmallVectorImpl<bool> &Succs,
                                        bool AggressiveUndef)
{
    Succs.resize(TI.getNumSuccessors());
    if (TI.getNumSuccessors() == 0)
        return;

    if (auto *BI = dyn_cast<BranchInst>(&TI))
    {
        if (BI->isUnconditional())
        {
            Succs[0] = true;
            return;
        }

        // NOTE: we always make all successors feasible for conditional branch
        Succs[0] = Succs[1] = true;
        return;
    }

    if (TI.isExceptionalTerminator())
    {
        Succs.assign(Succs.size(), true);
        return;
    }

    if (isa<IndirectBrInst>(TI))
    {
        Succs.assign(Succs.size(), true);
        return;
    }

    SwitchInst &SI = cast<SwitchInst>(TI);
    auto *C = dyn_cast_or_null<Constant>(SI.getCondition());
    if (!C || !isa<ConstantInt>(C))
    {
        // All destinations are executable!
        Succs.assign(TI.getNumSuccessors(), true);
        return;
    }
    SwitchInst::CaseHandle Case = *SI.findCaseValue(cast<ConstantInt>(C));
    Succs[Case.getSuccessorIndex()] = true;
}
#else
void TaintSolver::getFeasibleSuccessors(TerminatorInst &TI, SmallVectorImpl<bool> &Succs,
                                        bool AggressiveUndef)
{
    Succs.resize(TI.getNumSuccessors());
    if (TI.getNumSuccessors() == 0)
        return;

    if (auto *BI = dyn_cast<BranchInst>(&TI))
    {
        if (BI->isUnconditional())
        {
            Succs[0] = true;
            return;
        }

        // NOTE: we always make all successors feasible for conditional branch
        Succs[0] = Succs[1] = true;
        return;
    }

    if (TI.isExceptional())
    {
        Succs.assign(Succs.size(), true);
        return;
    }

    if (isa<IndirectBrInst>(TI))
    {
        Succs.assign(Succs.size(), true);
        return;
    }

    SwitchInst &SI = cast<SwitchInst>(TI);
    auto *C = dyn_cast_or_null<Constant>(SI.getCondition());
    if (!C || !isa<ConstantInt>(C))
    {
        // All destinations are executable!
        Succs.assign(TI.getNumSuccessors(), true);
        return;
    }
    SwitchInst::CaseHandle Case = *SI.findCaseValue(cast<ConstantInt>(C));
    Succs[Case.getSuccessorIndex()] = true;
}
#endif

bool TaintSolver::isEdgeFeasible(BasicBlock *From, BasicBlock *To, bool AggressiveUndef)
{
    SmallVector<bool, 16> SuccFeasible;
#if LLVM_VERSION_MAJOR >= 8
    Instruction *TI = From->getTerminator();
#else
    TerminatorInst *TI = From->getTerminator();
#endif
    getFeasibleSuccessors(*TI, SuccFeasible, AggressiveUndef);

    for (unsigned i = 0, e = TI->getNumSuccessors(); i != e; ++i)
        if (TI->getSuccessor(i) == To && SuccFeasible[i])
            return true;

    return false;
}

#if LLVM_VERSION_MAJOR >= 8
void TaintSolver::visitTerminator(Instruction &TI)
{
    SmallVector<bool, 16> SuccFeasible;
    getFeasibleSuccessors(TI, SuccFeasible, true);

    BasicBlock *BB = TI.getParent();

    // Mark all feasible successors executable...
    for (unsigned i = 0, e = SuccFeasible.size(); i != e; ++i)
        if (SuccFeasible[i])
            markEdgeExecutable(BB, TI.getSuccessor(i));
}
#else
void TaintSolver::visitTerminator(TerminatorInst &TI)
{
    SmallVector<bool, 16> SuccFeasible;
    getFeasibleSuccessors(TI, SuccFeasible, true);

    BasicBlock *BB = TI.getParent();

    // Mark all feasible successors executable...
    for (unsigned i = 0, e = SuccFeasible.size(); i != e; ++i)
        if (SuccFeasible[i])
            markEdgeExecutable(BB, TI.getSuccessor(i));
}
#endif

void TaintSolver::visitPHINode(PHINode &PN)
{
    // The lattice function may store more information on a PHINode than could be
    // computed from its incoming values.  For example, SSI form stores its sigma
    // functions as PHINodes with a single incoming value.
    if (LatticeFunc->IsSpecialCasedPHI(&PN))
    {
        DenseMap<TaintLatticeKey, TaintLatticeVal> ChangedValues;
        LatticeFunc->ComputeInstructionState(PN, ChangedValues, *this);
        for (auto &ChangedValue : ChangedValues)
            if (ChangedValue.second != LatticeFunc->getUntrackedVal())
                UpdateState(std::move(ChangedValue.first),
                            std::move(ChangedValue.second));
        return;
    }

    TaintLatticeKey Key = getLatticeKeyFromValue(&PN);
    TaintLatticeVal PNIV = getValueState(Key);
    TaintLatticeVal Overdefined = LatticeFunc->getOverdefinedVal();

    // If this value is already overdefined (common) just return.
    if (PNIV == Overdefined || PNIV == LatticeFunc->getUntrackedVal())
        return;  // Quick exit

    // Super-extra-high-degree PHI nodes are unlikely to ever be interesting,
    // and slow us down a lot.  Just mark them overdefined.
    if (PN.getNumIncomingValues() > 64)
    {
        UpdateState(Key, Overdefined);
        return;
    }

    // Look at all of the executable operands of the PHI node.  If any of them
    // are overdefined, the PHI becomes overdefined as well.  Otherwise, ask the
    // transfer function to give us the merge of the incoming values.
    for (unsigned i = 0, e = PN.getNumIncomingValues(); i != e; ++i)
    {
        // If the edge is not yet known to be feasible, it doesn't impact the PHI.
        if (!isEdgeFeasible(PN.getIncomingBlock(i), PN.getParent(), true))
            continue;

        // Merge in this value.
        TaintLatticeVal OpVal =
            getValueState(getLatticeKeyFromValue(PN.getIncomingValue(i)));
        if (OpVal != PNIV)
            PNIV = LatticeFunc->MergeValues(PNIV, OpVal);

        if (PNIV == Overdefined)
            break;  // Rest of input values don't matter.
    }

    // Update the PHI with the compute value, which is the merge of the inputs.
    UpdateState(Key, PNIV);
}

void TaintSolver::visitInst(Instruction &I)
{
    // PHIs are handled by the propagation logic, they are never passed into the
    // transfer functions.
    if (auto *PN = dyn_cast<PHINode>(&I))
        return visitPHINode(*PN);

    // Otherwise, ask the transfer function what the result is.  If this is
    // something that we care about, remember it.
    DenseMap<TaintLatticeKey, TaintLatticeVal> ChangedValues;
    LatticeFunc->ComputeInstructionState(I, ChangedValues, *this);
    for (auto &ChangedValue : ChangedValues)
        if (ChangedValue.second != LatticeFunc->getUntrackedVal())
            UpdateState(ChangedValue.first, ChangedValue.second);

#if LLVM_VERSION_MAJOR >= 8
    if (I.isTerminator())
        visitTerminator(I);
#else
    if (TerminatorInst *TI = dyn_cast<TerminatorInst>(&I))
        visitTerminator(*TI);
#endif
}

void TaintSolver::Solve()
{
    // Process the work lists until they are empty!
    while (!BBWorkList.empty() || !ValueWorkList.empty())
    {
        // Process the value work list.
        while (!ValueWorkList.empty())
        {
            Value *V = ValueWorkList.back();
            ValueWorkList.pop_back();

            // "V" got into the work list because it made a transition. See if any
            // users are both live and in need of updating.
            for (User *U : V->users())
                if (auto *Inst = dyn_cast<Instruction>(U))
                    if (BBExecutable.count(Inst->getParent()))  // Inst is executable?
                        visitInst(*Inst);
        }

        // Process the basic block work list.
        while (!BBWorkList.empty())
        {
            BasicBlock *BB = BBWorkList.back();
            BBWorkList.pop_back();

            // Notify all instructions in this basic block that they are newly
            // executable.
            for (Instruction &I : *BB)
                visitInst(I);
        }
    }
}

void TaintSolver::Print(raw_ostream &OS) const
{
    if (ValueState.empty())
        return;

    TaintLatticeKey Key;
    TaintLatticeVal LV;

    OS << "ValueState:\n";
    for (auto &Entry : ValueState)
    {
        std::tie(Key, LV) = Entry;
        if (LV == LatticeFunc->getUntrackedVal())
            continue;
        OS << "\t";
        LatticeFunc->PrintLatticeVal(LV, OS);
        OS << ": ";
        LatticeFunc->PrintLatticeKey(Key, OS);
        OS << "\n";
    }
}

static bool runTP(Module &M)
{
    std::unordered_map<Value *, SmallPtrSet<Value *, 16>> ValueDependencyMap;

    for (Function &F : M)
    {
        for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i)
        {
            Instruction *I = &*i;
            if (auto *GEPI = dyn_cast<GetElementPtrInst>(I))
            {
                ValueDependencyMap[GEPI].insert(GEPI->getPointerOperand());
            }
            else if (auto *PN = dyn_cast<PHINode>(I))
            {
                for (Value *V : PN->incoming_values())
                {
                    ValueDependencyMap[PN].insert(V);
                }
            }
            else if (auto *SI = dyn_cast<SelectInst>(I))
            {
                ValueDependencyMap[SI].insert(
                    { SI->getTrueValue(), SI->getFalseValue() });
            }
            else if (auto *CI = dyn_cast<CastInst>(I))
            {
                ValueDependencyMap[CI].insert(CI->getOperand(0));
            }
            else if (auto *LI = dyn_cast<LoadInst>(I))
            {
                if (LI->getType()->isPointerTy())
                {
                    ValueDependencyMap[LI].insert(LI->getPointerOperand());
                }
            }
            else
            {
                // TODO: handle other instructions
            }
        }
    }

    // Our custom lattice function and solver.
    TaintLatticeFunc Lattice;
    TaintSolver Solver(&Lattice, ValueDependencyMap);

    for (Function &F : M)
    {
#define HANDLE_TAINT_SOURCE(FUNC_NAME, ARGS)                                             \
    do                                                                                   \
    {                                                                                    \
        if (F.getName().equals(FUNC_NAME))                                               \
            for (User * U : F.users())                                                   \
                if (Instruction *Inst = dyn_cast<Instruction>(U))                        \
                    Solver.MarkBlockExecutable(Inst->getParent());                       \
    } while (false)

#include "Taint.def"
#undef HANDLE_TAINT_SOURCE
    }

    // Solver our custom lattice. In doing so, we will also get tainted instructions
    Solver.Solve();

    // Attach metadata to the tainted instructions
    for (Function &F : M)
    {
        for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i)
        {
            Instruction *I = &*i;
            SmallVector<Metadata *, 4> TaintsMetadatas;

            std::vector<Value *> Taints;
            if (auto *V = dyn_cast<Value>(I))
            {
                Taints.push_back(V);
            }
            for (Use &U : I->operands())
            {
                if (isa<Constant>(U))
                    continue;
                Value *V = U.get();
                Taints.push_back(V);
            }

            for (auto *V : Taints)
            {
                auto TLK = TaintLatticeKey(V, IPOGrouping::Register);
                TaintLatticeVal TLV = Solver.getExistingValueState(TLK);
                if (TLV.isTainted() && reachable(TLV.getTaintedAtInsts(), I))
                {
                    TaintsMetadatas.push_back(ValueAsMetadata::get(V));
                }
            }

            if (!TaintsMetadatas.empty())
            {
                MDNode *TaintMDNode = MDNode::get(M.getContext(), TaintsMetadatas);
                I->setMetadata(MD_TAINT, TaintMDNode);
            }
        }
    }
    return false;
}

bool TaintPropagationLegacyPass::runOnModule(Module &M)
{
    if (skipModule(M))
        return false;
    return runTP(M);
}

char TaintPropagationLegacyPass::ID = 0;
