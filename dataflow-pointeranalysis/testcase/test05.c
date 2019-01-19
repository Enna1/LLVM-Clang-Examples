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
    int (*t_fptr[2])(int, int) ;
    int (*q_fptr[1])(int, int) ;
    int (*r_fptr[2])(int, int) ;

    int op1=1, op2=2;

    if (x >= 3) {
       t_fptr[1] = a_fptr;
    } 
    if (x >= 4) {
       t_fptr[1] = s_fptr;
    }
    if (x >= 5) {
       q_fptr[0] = t_fptr[1];
    }
    if (x >= 6) 
       r_fptr[1] = q_fptr[0];
    

    if (t_fptr[1] != NULL) {
       unsigned result = r_fptr[1](op1, op2);
    }  
   return 0;
}

/// 33 : plus, minus
