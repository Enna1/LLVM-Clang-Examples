#include <stdlib.h>
int plus(int a, int b) {
   return a+b;
}

int minus(int a, int b) {
   return a-b;
}

int clever(int a, int b, int (*a_fptr)(int, int)) {
    return a_fptr(a, b);
}
struct fptr
{
int (*p_fptr)(int, int);
};

int moo(char x, int op1, int op2) {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;
    struct fptr * t_fptr=(struct fptr *)malloc(sizeof(struct fptr));
    int (*af_ptr)(int ,int ,int(*)(int, int)) = clever;
    unsigned result = 0;
    if (x == '+') {
       t_fptr->p_fptr = a_fptr;
    }else
    {
       t_fptr->p_fptr = s_fptr;
    }
    result= af_ptr(op1, op2, t_fptr->p_fptr);
    return result;
}

/// 11 : plus, minus
/// 21 : malloc
/// 30 : clever
