# Run an LLVM Pass with Clang

If you want to run an LLVM pass while compiling a program, you need to do the following:

1. Compile each source file to bitcode with `clang -c -emit-llvm code.c`
2. Run your pass  with `opt -load yourpass.so -yourpass code.bc -o code_out.bc`
3. Run the rest of the standard optimizations with `opt -O3 code_out.bc -o code_opt.bc`
4. Compile the optimized bitcode into assembly with llc and then use your favorite assembler and linker to get the rest of the way to an executable



The code in this directory is a trick to run an LLVM Pass with Clang, just a snippet of code like this at the end of your pass code:

```c++
static void registerHelloPass(const PassManagerBuilder &, legacy::PassManagerBase &PM)
{
    PM.add(new YourPass());
}
static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                                             registerHelloPass);
```

Build:

```
cd clang-pass
cmake .
make
```

Run:

```
clang -Xclang -load -Xclang libhello.so foo.c
```



Reference: https://www.cs.cornell.edu/~asampson/blog/clangpass.html