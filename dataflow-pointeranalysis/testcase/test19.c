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
int (*clever(int a, int b, int (*a_fptr)(int, int), int(*b_fptr)(int, int) ))(int, int) {
   return b_fptr;
}
int moo(char x, int op1, int op2) {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;
    int (* (*goo_ptr)(int, int, int (*)(int, int), int(*)(int, int)))(int, int)=foo;
    int (*t_fptr)(int, int) = 0;

    if(x == '+')
    {
        t_fptr = goo_ptr(op1, op2, a_fptr, s_fptr);
    }else
    {
        goo_ptr=clever;
        t_fptr = goo_ptr(op1, op2, s_fptr, a_fptr);
    }
    t_fptr(op1, op2);   
    return 0;
}


// 24 : foo
// 28 : clever  
// 30 : plus
