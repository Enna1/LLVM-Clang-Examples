# C-AST-Interpreter

LLVM/Clang Version: 5.0.1



A basic tiny C language interpreter based on Clang.

We support a subset of C language constructs, as follows: 

- Type: int | char | void | *
- Operator: * | + | - | * | / | < | > | == | = | [ ] 
- Statements: IfStmt | WhileStmt | ForStmt | DeclStmt 
- Expr : BinaryOperator | UnaryOperator | DeclRefExpr | CallExpr | CastExpr 
- Support 4 external functions int GET(), void * MALLOC(int), void FREE (void *), void PRINT(int).

A simple program the interpreter is able to interpreter : 

```c
extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a;
   a = GET();
   PRINT(a);
}

```



Build & Use:

```bash
$ cd c-ast-interpreter
$ cmake .
$ make
$ ./ast-interpreter "`cat test/test0.c`"
```




