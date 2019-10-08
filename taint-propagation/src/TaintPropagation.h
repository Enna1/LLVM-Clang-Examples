//===- CalledValuePropagation.h - Propagate called values -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a transformation that attaches !callees metadata to
// indirect call sites. For a given call site, the metadata, if present,
// indicates the set of functions the call site could possibly target at
// run-time. This metadata is added to indirect call sites when the set of
// possible targets can be determined by analysis and is known to be small. The
// analysis driving the transformation is similar to constant propagation and
// makes uses of the generic sparse propagation solver.
//
//===----------------------------------------------------------------------===//

#ifndef TAINTPROPAGATION_H
#define TAINTPROPAGATION_H

#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"

class TaintPropagationLegacyPass : public llvm::ModulePass
{
public:
    static char ID;

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        // AU.addRequired<llvm::UnifyFunctionExitNodes>();
    }

    TaintPropagationLegacyPass() : ModulePass(ID) {}

    bool runOnModule(llvm::Module &M) override;
};
#endif  // TAINTPROPAGATION_H
