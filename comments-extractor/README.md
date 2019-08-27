# CommentsExtractor

CommentsExtractor is a simple tool based on clang's libtooling to grab all the comments from C source code.

## Build & Usage

```bash
$ cd /path/to/CommentsExtractor
$ mkdir build
$ cd build
$ cmake ..
$ make
```

### Single source file

```bash
$ cat ../testcases/foo.c 
/*
  multi-line
  comment
*/

/// Documentation comment

int foo(int x)
{
    int result = (x / 123);  // end of line comment
    return result;
}
/** this is a block comment */

int main(int argc, char** argv)
{
    return 0;
}

$ ./comments-extractor ../testcases/foo.c -extra-arg="-fparse-all-comments" --
Comment: < multi-line comment > at </home/test/Documents/comments-extractor/build/../testcases/test.c:1:1, line:4:3>

Comment: < Documentation comment > at </home/test/Documents/comments-extractor/build/../testcases/test.c:6:1, col:26>

Comment: < end of line comment > at </home/test/Documents/comments-extractor/build/../testcases/test.c:10:30, col:52>

Comment: < this is a block comment > at </home/test/Documents/comments-extractor/build/../testcases/test.c:13:1, col:31>

```

The libTooling command-line parser (`CommonOptionsParser`) supports providing compiler flags on the command line, following the special flag `--`. In this case, the `--` at the end means  it prevents the tool from searching for a compilation database while analyzing the file.

### Project with multiple files

To grab all the comments from a project that has multiple files, we’d have to use a compilation database.

Currently [CMake](https://cmake.org/) (since 2.8.5) supports generation of compilation databases for Unix Makefile builds (Ninja builds in the works) with the option `CMAKE_EXPORT_COMPILE_COMMANDS`.

For projects on Linux, there is an alternative to intercept compiler calls with a tool called [Bear](https://github.com/rizsotto/Bear).

Take this project `CommentsExtractor`as an example for using  CMake to generate compilation databases:

```bash
$ cd /path/to/CommentsExtractor/build
$ cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
```

And you'll find `compile_commands.json` in `build` dir.

Use `-p` option to read a compile command database.

```bash
$ ./comments-extractor -p=./ ../src/CommentsExtractor.cpp -extra-arg="-fparse-all-comments"
```

## Reference

1. https://danielbeard.io/2016/04/19/clang-frontend-action-part-1.html
2. http://clang.llvm.org/docs/JSONCompilationDatabase.html
3. https://eli.thegreenplace.net/2014/05/21/compilation-databases-for-clang-based-tools
4. https://github.com/rizsotto/Bear

