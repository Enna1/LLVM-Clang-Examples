#include <stdlib.h>
struct fptr
{
int (*p_fptr)(int, int);
};
int plus(int a, int b) {
   return a+b;
}

int minus(int a, int b) {
   return a-b;
}

int (*foo(int a, int b, int (*a_fptr)(int, int), int(*b_fptr)(int, int) ))(int, int) {
   return b_fptr;
}

int clever(int a, int b, int (*a_fptr)(int, int), struct fptr b_fptr) {
   int (*s_fptr)(int, int);
   s_fptr = foo(a, b, a_fptr, b_fptr.p_fptr);
   return s_fptr(a, b);
}


int moo(char x, int op1, int op2) {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;
    struct fptr t_fptr= {0};

    if (x == '+') {
       t_fptr.p_fptr = a_fptr;
    } 
    else if (x == '-') {
       t_fptr.p_fptr = s_fptr;
    }

    unsigned result = clever(op1, op2, a_fptr, t_fptr);
    return 0;
}


/// 20 : foo
/// 21 : plus, minus 
/// 37 : clever 
