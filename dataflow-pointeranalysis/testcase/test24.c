#include <stdlib.h>
struct fptr
{
int (*p_fptr)(int, int);
};
int plus(int a, int b) {
   return a+b;
}

int minus(int a,int b)
{
    return a-b;
}

int foo(int a,int b,struct fptr af_ptr)
{
	return af_ptr.p_fptr(a,b);
}
void make_no_alias(struct fptr a)
{
    a.p_fptr=plus;
}
int clever() {
  
    int (*af_ptr)(int ,int ,struct fptr)=0;
    struct fptr tf_ptr={0};
    tf_ptr.p_fptr=minus;
    af_ptr=foo;
    make_no_alias(tf_ptr);
    unsigned result = af_ptr(1, 2,tf_ptr);
    return 0;
}
// 17 : minus
// 29 : make_no_alias
// 30 : foo