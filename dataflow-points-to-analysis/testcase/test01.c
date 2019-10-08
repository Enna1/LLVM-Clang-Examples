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
    int (*t_fptr)(int, int) = minus;

    int op1=1, op2=2;

    if (x == 3) {
       t_fptr = a_fptr;
    } 

    if (t_fptr != NULL) {
       unsigned result = t_fptr(op1, op2);
    }  
   return 0;
}

// 22 : plus, minus 
