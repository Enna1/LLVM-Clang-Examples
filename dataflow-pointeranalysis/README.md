# Point-to Analysis via Dataflow

### Description

LLVM/Clang Version: 5.0.1

A LLVM pass that implements a flow-sensitive, field- and context-insensitive point-to analysis via dataflow, computing the points-to set for each variable at each distinct program point.

source code is in `src` directory, `testcase`  dir consists of 35 testcases.

### Input

To test this pass, you should build LLVM bytecode from the test cases using the following commands: 

```shell
$ clang -emit-llvm -c -O0 -g3 infile.c -o infile.bc
$ opt -mem2reg infile.bc -o infile-m2r.bc
```

### Output

Print the callee functions at every call instructions.

### Build

build this pass.

```shell
$ cd /path-to-dataflow-pointeranalysis/src
$ cmake .
$ make
```

### Example

Take `testcase/test00.c` as a simple example.

```c
// test00.c
#include <stdlib.h>

int plus(int a, int b) {
   return a+b;
}

int minus(int a,int b)
{
    return a-b;
}

int foo(int a,int b,int(* a_fptr)(int, int))
{
    return a_fptr(a,b);
}


int moo(char x)
{
    int (*af_ptr)(int ,int ,int(*)(int, int))=foo;
    int (*pf_ptr)(int,int)=0;
    if(x == '+'){
        pf_ptr=plus;
        af_ptr(1,2,pf_ptr);
        pf_ptr=minus;
    }
    af_ptr(1,2,pf_ptr);
    return 0;
}

//14 : plus, minus
//24 : foo
//27 : foo

```



First, build this dataflow-pointeranalysis pass.

```shell
$ cd /path-to-dataflow-pointeranalysis/src
$ cmake .
$ make
```

Then, build unoptimized LLVM bytecode from test00.c

```shell
$ cd /path-to-dataflow-pointeranalysis/testcase
$ clang -emit-llvm -c -O0 -g3 test00.c -o test00.bc
$ opt -mem2reg test00.bc -o test00-m2r.bc
```

Finally, run this dataflow-pointeranalysis pass, and get the output.

```shell
$ cd /path-to-dataflow-pointeranalysis/src
$ ./df-pointeranalysis ../testcase/test00-m2r.bc
14 : plus, minus
24 : foo
27 : foo
```

