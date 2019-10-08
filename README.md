# LLVM-Clang-Examples

A collection of code based on LLVM/Clang compilation libraries and tools.

There are also some useful tutorials about LLVM/clang in directory `tutorials/`.

To install llvm, you can use the installer script `install-llvm-5.0.1.sh` that can be found in directory `utils/`.

E.g. use `$ ./install-llvm-5.0.1.sh 4 ~/` to build llvm-5.0.1 using 4 cores in your home directory.

## Contents

- [Tutorials](#tutorials)
- [Custom class supporting LLVM RTTI](#custom-class-supporting-llvm-rtti)
- [C AST Interpreter](#c-ast-interpreter)
- [Comments Extractor](#comments-extractor)
- [Dataflow Points-to Analysis](#dataflow-points-to-analysis)
- [Use CalledValuePropagationPass in your tool](#use-calledvaluepropagationpass-in-your-tool)
- [Break ConstantExpr](#break-constantexpr)
- [Unify Function Exits](#unify-function-exits)
- [Run an LLVM Pass with Clang](#run-an-llvm-pass-with-clang)
- [JIT for brainfuck](#jit-for-brainfuck)
- [Taint Propagation](#taint-propagation)

## Tutorials

- [LLVM IR Tutorial - Phis, GEPs and other things, oh my!](https://github.com/Enna1/LLVM-Clang-Examples/tree/master/tutorials/Tutorial-Bridgers-LLVM_IR_tutorial.pdf)
- [How to Write a Clang Static Analyzer Checker in 24 Hours](https://github.com/Enna1/LLVM-Clang-Examples/tree/master/tutorials/Clang-Static-Analyzer-Checker24Hours.pdf)
- [Scalar Evolution Demystified](https://github.com/Enna1/LLVM-Clang-Examples/blob/master/tutorials/Absar-ScalarEvolution.pdf)

## Custom class supporting LLVM RTTI

Sample code used to demonstrate how to set up LLVM-Style RTTI for your custom class hierarchy.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/custom-class-support-llvm-rtti

## C AST Interpreter

A basic tiny C language interpreter based on Clang.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/c-ast-interpreter

## Comments Extractor

A simple tool based on clang's libtooling to grab all the comments from C source code.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/comments-extractor

## Dataflow Points-to Analysis

An LLVM pass that implements a flow-sensitive, field- and context-insensitive points-to analysis via dataflow, computing the points-to set for each variable at each distinct program point.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/dataflow-points-to-analysis

## Use CalledValuePropagationPass in your tool

Sample code used to demonstrate how to call CalledValuePropagationPass in your own tool based LLVM.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/use-calledvaluepropagation-in-your-tool

## Break ConstantExpr

This BreakConstantExpr is a module pass that converts all constant expressions into instructions. 

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/break-constantexpr

## Unify Function Exits

This UnifyFunctionExits module pass is just a wrapper for UnifyFunctionExitNodes function pass, used to ensure that functions have at most one return instruction in them.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/unify-functionexits

## Run an LLVM Pass with Clang

Sample code used to demonstrate how to run your LLVM Pass with Clang.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/clang-pass

## JIT for brainfuck

A JIT using LLVM for brainfuck language.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/building-a-JIT-for-BF

## Taint Propagation

An LLVM transform pass implements a flow-sensitive, field- and context-insensitive taint propagation based on classic dataflow analysis. 

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/taint-propagation

