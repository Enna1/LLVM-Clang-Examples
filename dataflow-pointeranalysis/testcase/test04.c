#include <stdlib.h>
int plus(int a, int b) {
   return a+b;
}

int minus(int a, int b) {
   return a-b;
}
int foo(int a, int b, int (*a_fptr)(int, int)) {
    return a_fptr(a, b);
}

int clever(int x) {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;
    int (*t_fptr)(int, int) = 0;
    int (*q_fptr)(int, int) = 0;
    int (*r_fptr)(int, int) = 0;
    int (*af_ptr)(int ,int ,int(*)(int, int)) = foo;

    int op1=1, op2=2;

    if (x >= 4) {
       t_fptr = s_fptr;
    }
    af_ptr(op1,op2,t_fptr);
    if (x >= 5) {
       t_fptr = a_fptr;
       q_fptr = t_fptr;
    }    

    if (t_fptr != NULL) {
       unsigned result = af_ptr(op1,op2,q_fptr);
    }  
   return 0;
}

/// 10 : plus, minus
/// 26 : foo
/// 33 : foo
