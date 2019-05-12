# LLVM-Clang-Examples

A collection of code based on LLVM/Clang compilation libraries and tools. There are also some useful tutorials about LLVM/clang in directory `tutorials/`.

To install llvm, you can use the installer script `install-llvm-5.0.1.sh` that can be found in directory `utils/`.

E.g. use `$ ./install-llvm-5.0.1.sh 4 ~/` to build llvm-5.0.1 using 4 cores in your home directory.

## Contents

- [tutorials](#tutorials)
- [custom-class-support-llvm-rtti](#custom-class-support-llvm-rtti)
- [c-ast-interpreter](#c-ast-interpreter)
- [dataflow-pointer-analysis](#dataflow-pointer-analysis)
- [use-calledvaluepropagation-in-your-tool](#use-calledvaluepropagation-in-your-tool)
- [break-constantexpr](#break-constantexpr)
- [unify-functionexits](#unify-functionexits)

## tutorials

- [LLVM IR Tutorial - Phis, GEPs and other things, oh my!](https://github.com/Enna1/LLVM-Clang-Examples/tree/master/tutorials/Tutorial-Bridgers-LLVM_IR_tutorial.pdf)
- [How to Write a Clang Static Analyzer Checker in 24 Hours](https://github.com/Enna1/LLVM-Clang-Examples/tree/master/tutorials/Clang-Static-Analyzer-Checker24Hours.pdf)

## custom-class-support-llvm-rtti

Sample code used to demonstrate how to set up LLVM-Style RTTI for your custom class hierarchy.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/custom-class-support-llvm-rtti

## c-ast-interpreter

A basic tiny C language interpreter based on Clang.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/c-ast-interpreter

## dataflow-pointer-analysis

A LLVM pass that implements a flow-sensitive, field- and context-insensitive point-to analysis via dataflow, computing the points-to set for each variable at each distinct program point.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/dataflow-pointeranalysis

## use-calledvaluepropagation-in-your-tool

Sample code used to demonstrate how to call CalledValuePropagationPass in your own tool based LLVM.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/use-calledvaluepropagation-in-your-tool

## break-constantexpr

This BreakConstantExpr is a module pass that converts all constant expressions into instructions. 

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/break-constantexpr

## unify-functionexits

This UnifyFunctionExits module pass is just a wrapper for UnifyFunctionExitNodes function pass, used to ensure that functions have at most one return instruction in them.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/unify-functionexits