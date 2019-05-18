#include "BreakConstantExpr.h"
using namespace llvm;

static ConstantExpr* hasConstantExpr(Value* V)
{
    if (ConstantExpr* CE = dyn_cast<ConstantExpr>(V))
    {
        return CE;
    }
    return nullptr;
}

bool BreakConstantExpr::runOnModule(Module& module)
{
    bool modified = false;
    for (Module::iterator F = module.begin(), E = module.end(); F != E; ++F)
    {
        // Worklist of values to check for constant GEP expressions
        std::vector<Instruction*> Worklist;

        // Initialize the worklist by finding all instructions that have one or more
        // operands containing a constant GEP expression.
        for (Function::iterator BB = (*F).begin(); BB != (*F).end(); ++BB)
        {
            for (BasicBlock::iterator i = BB->begin(); i != BB->end(); ++i)
            {
                // Scan through the operands of this instruction.  If it is a constant
                // expression GEP, insert an instruction GEP before the instruction.
                Instruction* I = &(*i);
                for (unsigned index = 0; index < I->getNumOperands(); ++index)
                {
                    if (hasConstantExpr(I->getOperand(index)))
                    {
                        Worklist.push_back(I);
                    }
                }
            }
        }

        // Determine whether we will modify anything.
        if (Worklist.size())
            modified = true;

        // While the worklist is not empty, take an item from it, convert the
        // operands into instructions if necessary, and determine if the newly
        // added instructions need to be processed as well.
        while (Worklist.size())
        {
            Instruction* I = Worklist.back();
            Worklist.pop_back();

            // Scan through the operands of this instruction and convert each into an
            // instruction.  Note that this works a little differently for phi
            // instructions because the new instruction must be added to the
            // appropriate predecessor block.
            if (PHINode* PHI = dyn_cast<PHINode>(I))
            {
                for (unsigned index = 0; index < PHI->getNumIncomingValues(); ++index)
                {
                    // For PHI Nodes, if an operand is a constant expression with a GEP,
                    // we want to insert the new instructions in the predecessor basic
                    // block.

                    // Note: It seems that it's possible for a phi to have the same
                    // incoming basic block listed multiple times; this seems okay as long
                    // the same value is listed for the incoming block.
                    Instruction* InsertPt = PHI->getIncomingBlock(index)->getTerminator();
                    if (ConstantExpr* CE = hasConstantExpr(PHI->getIncomingValue(index)))
                    {
                        // ConstantExpr member function 'getAsInstruction()' returns an
                        // Instruction which implements the same operation as this
                        // ConstantExpr.
                        Instruction* NewInst = CE->getAsInstruction();
                        NewInst->insertBefore(InsertPt);
                        for (unsigned i2 = index; i2 < PHI->getNumIncomingValues(); ++i2)
                        {
                            if ((PHI->getIncomingBlock(i2)) ==
                                PHI->getIncomingBlock(index))
                                PHI->setIncomingValue(i2, NewInst);
                        }
                        Worklist.push_back(NewInst);
                    }
                }
            }
            else
            {
                for (unsigned index = 0; index < I->getNumOperands(); ++index)
                {
                    // For other instructions, we want to insert instructions replacing
                    // constant expressions immediently before the instruction using the
                    // constant expression.
                    if (ConstantExpr* CE = hasConstantExpr(I->getOperand(index)))
                    {
                        // ConstantExpr member function 'getAsInstruction()' returns an
                        // Instruction which implements the same operation as this
                        // ConstantExpr.
                        Instruction* NewInst = CE->getAsInstruction();
                        NewInst->insertBefore(I);
                        I->replaceUsesOfWith(CE, NewInst);
                        Worklist.push_back(NewInst);
                    }
                }
            }
        }
    }
    return modified;
}

char BreakConstantExpr::ID = 0;
