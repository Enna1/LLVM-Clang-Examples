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
void make_simple_alias(struct fptr *a,struct fptr * b)
{
    a->p_fptr=b->p_fptr;
}
int clever() {
  
    int (*af_ptr)(int ,int ,struct fptr)=0;
    struct fptr tf_ptr={0};
    struct fptr sf_ptr={0};
    tf_ptr.p_fptr=minus;
    sf_ptr.p_fptr=plus;
    af_ptr=foo;
    make_simple_alias(&tf_ptr,&sf_ptr);
    unsigned result = af_ptr(1, 2,tf_ptr);
    return 0;
}
// 17 : plus
// 31 : make_simple_alias
// 32 : foo