# JIT for brainfuck using LLVM

### Description

LLVM/Clang Version: 7.0.0

Brainfuck is the ungodly creation of Urban Müller, whose goal was apparently to create a Turing-complete language for which he could write the smallest compiler ever, for the Amiga OS 2.0. See https://esolangs.org/wiki/Brainfuck for more information.

`bf-interpreter` dir implements a simple interpreter for brainfuck,  `bf-interpreter` dir implements a JIT for brainfuck using LLVM, `testcase`  dir consists of 14 testcases.

### Input

To run this JIT for brainfuck language , you should input some valid  brainfuck programs, `testcase`  dir consists of 14 testcases.

### Build

```shell
$ cd /building-a-JIT-for-BF/bf-jit
$ cmake .
$ make
```

### Example

Take `testcase/helloword1.bf` as a simple example.

```brainfuck
++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.
```

run with `testcase/helloword1.bf`, and get the output.

```shell
$ cd /building-a-JIT-for-BF/bf-jit
$ ./bf-jit ./testcase/helloworld1.bf
Hello World!
```

### References

1. https://eli.thegreenplace.net/2017/adventures-in-jit-compilation-part-1-an-interpreter/
2. https://llvm.org/docs/tutorial/BuildingAJIT1.html