# Use CalledValuePropagationPass in your tool

### Description

LLVM/Clang Version: 7.0.0

This is the sample code used to demonstrate how to call CalledValuePropagationPass in your tool.

About CalledValuePropagationPass , see https://inside-llvm-clang-soure-code.readthedocs.io/en/latest/called-value-propagation/index.html for detail.

source code is in `src` directory, `testcase`  dir consists of 1 testcase.

### Build & Test

Build this sample code.

```shell
$ cd /src
$ cmake .
$ make
```

Test this and get the output.

```shell
$ cd /src
$ ./test-cvp ../simple-arguments.ll
ugt, ule, 
```

In the sample code, we only get the functions name of indirect call sites could possibly target at. If you have a look at "FooPass.h", you will see the following code snippet:

```c++
bool runOnFunction(Function &F) override
{
    for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i)
    {
        Instruction *I = &(*i);
        if (auto *MD = I->getMetadata(LLVMContext::MD_callees))
        {
            for (auto &Op : MD->operands())
            {
                Function *Callee = mdconst::extract_or_null<Function>(Op);
                if (Callee)
                    errs() << Callee->getName() << ", ";
            }
            errs() << "\n";
        }
    }
    return false;
} 
```
Actually we can get the function pointer (`Function*`) for possible targets of a indirect call site, so it's easy to do a lot of things.