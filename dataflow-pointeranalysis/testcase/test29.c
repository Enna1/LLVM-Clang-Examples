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

int minus(int a, int b) {
   return a-b;
}
int clever(int a, int b, struct fptr * c_fptr, struct fptr * d_fptr) {
	int (*t_fptr)(int, int)=c_fptr->p_fptr;
	c_fptr->p_fptr=d_fptr->p_fptr;
	d_fptr->p_fptr=t_fptr;
    return t_fptr(a,b);
}
int foo(int a, int b, struct fsptr * c_fptr, struct fsptr * d_fptr) {
	struct fptr t_fptr=*(c_fptr->sptr);
	c_fptr->sptr=d_fptr->sptr;
    clever(a, b, c_fptr->sptr,&t_fptr);
    return t_fptr.p_fptr(a,b);
}
void moo(int x)
{
	struct fptr a_fptr;
	a_fptr.p_fptr=plus;
	struct fptr b_fptr;
	b_fptr.p_fptr=minus;

	struct fsptr s_fptr;
	s_fptr.sptr=&a_fptr;
	struct fsptr r_fptr;
	r_fptr.sptr=&b_fptr;

	struct fsptr* w_fptr=(struct fsptr*)malloc(sizeof(struct fsptr));
    
	*w_fptr=s_fptr;
	if(x>1)
	{
		 foo(1,x,w_fptr,&r_fptr);
	}else
	{
		w_fptr->sptr->p_fptr=plus;
	}
	foo(1,x,w_fptr,&s_fptr);
}

// 21 : minus, plus
// 26 : clever
// 27 : plus, minus
// 41 : malloc
// 46 : foo
// 51 : foo