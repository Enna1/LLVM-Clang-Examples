#include <stdlib.h>

int plus(int a, int b) {
   return a+b;
}

int minus(int a, int b) {
   return a-b;
}

int clever(int x) {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;
    int (*t_fptr[2])(int, int) = {0};

    int op1=1, op2=2;

    if (x == 3) {
       t_fptr[0] = a_fptr;
    } 
    if (x == 4) {
       t_fptr[0]= s_fptr;
    }
    

    if (t_fptr[0] != NULL) {
       unsigned result = t_fptr[0](op1, op2);
    }  
   return 0;
}

/// 27 : plus, minus
