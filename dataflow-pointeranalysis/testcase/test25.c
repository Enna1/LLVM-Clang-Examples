#include <stdlib.h>
struct fptr
{
int (*p_fptr)(int, int);
};
struct fsptr
{
struct fptr * sptr;
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
void make_alias(struct fsptr * a,struct fptr* b)
{
    a->sptr=b;
}
int clever() {
  
    int (*af_ptr)(int ,int ,struct fptr)=0;
    struct fptr tf_ptr={0};
    struct fptr mf_ptr={0};
    struct fsptr sf_ptr={0};
    sf_ptr.sptr=&mf_ptr;
    tf_ptr.p_fptr=plus;
    mf_ptr.p_fptr=minus;
    af_ptr=foo;
    make_alias(&sf_ptr,&tf_ptr);
    unsigned result = af_ptr(1, 2,*(sf_ptr.sptr));
    return 0;
}

// 21 : plus
// 37 : make_alias
// 38 : foo