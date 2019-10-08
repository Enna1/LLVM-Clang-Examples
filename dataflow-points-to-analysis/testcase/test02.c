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

int clever(int x) {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;
    struct fptr t_fptr={0};

    int op1=1, op2=2;

    if (x == 3) {
       t_fptr.p_fptr = a_fptr;
    } else {
       t_fptr.p_fptr = s_fptr;
    }

    if (t_fptr.p_fptr != NULL) {
       unsigned result = t_fptr.p_fptr(op1, op2);
    }  
   return 0;
}

/// 28 : plus, minus
