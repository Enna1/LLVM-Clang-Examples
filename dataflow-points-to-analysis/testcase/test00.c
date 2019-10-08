#include <stdlib.h>

int plus(int a, int b) {
   return a+b;
}

int minus(int a,int b)
{
    return a-b;
}

int foo(int a,int b,int(* a_fptr)(int, int))
{
    return a_fptr(a,b);
}


int moo(char x)
{
    int (*af_ptr)(int ,int ,int(*)(int, int))=foo;
    int (*pf_ptr)(int,int)=0;
    if(x == '+'){
        pf_ptr=plus;
        af_ptr(1,2,pf_ptr);
        pf_ptr=minus;
    }
    af_ptr(1,2,pf_ptr);
    return 0;
}

//14 : plus, minus
//24 : foo
//27 : foo

