# UnifyFunctionExits

This UnifyFunctionExits module pass is just a wrapper for UnifyFunctionExitNodes function pass.

UnifyFunctionExitNodes pass is used to ensure that functions have at most one return instruction in them.  It unifies all exit nodes of the CFG by creating a new BasicBlock, and converting all returns to unconditional branches to this new basic block. 
Here are functions provided by UnifyFunctionExitNodes pass to keeps track of which node is the new exit node of the CFG.

```c++
// getReturn|Unwind|UnreachableBlock - Return the new single (or nonexistent)
// return, unwind, or unreachable  basic blocks in the CFG.
//
BasicBlock *getReturnBlock() const { return ReturnBlock; }
BasicBlock *getUnwindBlock() const { return UnwindBlock; }
BasicBlock *getUnreachableBlock() const { return UnreachableBlock; }
```

