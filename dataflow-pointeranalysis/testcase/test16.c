#include <stdlib.h>
int plus(int a, int b) {
   return a+b;
}

int minus(int a, int b) {
   return a-b;
}

int (*foo(int a, int b, int (*a_fptr)(int, int), int(*b_fptr)(int, int) ))(int, int) {
   return a_fptr;
}

int clever(int a, int b, int (*a_fptr)(int, int), int(**b_fptr)(int, int)) {
   int (*s_fptr)(int, int);
   s_fptr = foo(a, b, a_fptr, *b_fptr);
   return s_fptr(a, b);
}


int moo(char x, int op1, int op2) {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;
    int (**t_fptr)(int, int) = (int (**)(int, int))malloc(sizeof(int (*)(int, int)));

    if (x == '+') {
       *t_fptr = a_fptr;
    } 
    else if (x == '-') {
       *t_fptr = s_fptr;
    }
    unsigned result = clever(op1, op2, a_fptr, t_fptr);
    return 0;
}


/// 16 : foo
/// 17 : plus
/// 24 : malloc
/// 32 : clever 
