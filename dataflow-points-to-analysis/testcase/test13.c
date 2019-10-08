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

int clever(int a, int b, struct fptr a_fptr) {
    return a_fptr.p_fptr(a, b);
}


int moo(char x, int op1, int op2) {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;
    struct fptr t_fptr= {0};

    if (x == '+') {
       t_fptr.p_fptr = a_fptr;
    } 
    else if (x == '-') {
       t_fptr .p_fptr= s_fptr;
    }

    unsigned result = clever(op1, op2, t_fptr);
    return 0;
}

/// 15 : plus, minus
/// 31 : clever
