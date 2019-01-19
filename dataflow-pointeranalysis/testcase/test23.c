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

void make_simple_alias(int (**af_ptr)(int,int),int (**bf_ptr)(int,int) )
{
	*af_ptr=*bf_ptr;
}

int moo(char x)
{
    int (*af_ptr)(int ,int ,int(*)(int, int))=foo;
    int (**pf_ptr)(int,int)=(int (**)(int,int))malloc(sizeof(int (*)(int,int)));
    int (**mf_ptr)(int,int)=(int (**)(int,int))malloc(sizeof(int (*)(int,int)));
    	*mf_ptr=minus;
    if(x == '+'){
        *pf_ptr=plus;
        af_ptr(1,2,*pf_ptr);
        make_simple_alias(mf_ptr,pf_ptr);
    }
    af_ptr(1,2,*mf_ptr);
    return 0;
}

// 14 : plus, minus
// 25 : malloc
// 26 : malloc
// 30 : foo
// 31 : make_simple_alias
// 33 : foo