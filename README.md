# LLVM-Clang-Examples

A collection of code based on LLVM/Clang compilation libraries and tools.

To install llvm, you can use the installer script `install-llvm-5.0.1.sh` that can be found in directory `utils/`.

E.g. use `$ ./install-llvm-5.0.1.sh 4 ~/` to build llvm-5.0.1 using 4 cores in your home directory.

## Contents

- [custom-class-support-llvm-rtti](#custom-class-support-llvm-rtti)
- [c-ast-interpreter](#c-ast-interpreter)
- [dataflow-pointer-analysis](#dataflow-pointer-analysis)

## custom-class-support-llvm-rtti

Sample code used to demonstrate how to set up LLVM-Style RTTI for your custom class hierarchy.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/custom-class-support-llvm-rtti

## c-ast-interpreter

A basic tiny C language interpreter based on Clang.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/c-ast-interpreter

## dataflow-pointer-analysis

A LLVM pass that implements a flow-sensitive, field- and context-insensitive point-to analysis via dataflow, computing the points-to set for each variable at each distinct program point.

For detail, see https://github.com/Enna1/LLVM-Clang-Examples/tree/master/dataflow-pointeranalysis