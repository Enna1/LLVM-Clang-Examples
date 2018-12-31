# Set up LLVM-Style RTTI for your custom class

LLVM/Clang Version: 5.0.1

About LLVM-Style RTTI, see https://inside-llvm-clang-soure-code.readthedocs.io/en/latest/beginning/RTTI-in-LLVM.html for detail.

This is the sample code used to demonstrate how to set up LLVM-Style RTTI for your custom class.

Here is the class hierarchy in sample code.

```
| Shape
  | Square
    | SpecialSquare
  | Circle
```

Build & Test:

```bash
$ cd custom-class-support-llvm-rtti
$ make
$ ./test
```


