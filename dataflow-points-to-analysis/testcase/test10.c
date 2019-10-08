#include <stdlib.h>
struct fptr
{
	int (*p_fptr)(int, int);
};
struct fsptr
{
struct fptr * sptr;
};
struct wfsptr
{
	struct fsptr * wfptr;
};
int plus(int a, int b) {
   return a+b;
}

int minus(int a, int b) {
   return a-b;
}
void foo(int x)
{
	struct fptr a_fptr;
	struct fsptr s_fptr;
	struct wfsptr* w_fptr=(struct wfsptr*)malloc(sizeof(struct wfsptr));
	s_fptr.sptr=&a_fptr;
	w_fptr->wfptr=&s_fptr;
	if(x>1)
	{
		 a_fptr.p_fptr=minus;
		 x=s_fptr.sptr->p_fptr(1,x);
		 a_fptr.p_fptr=plus;
	}else
	{
		w_fptr->wfptr->sptr->p_fptr=plus;
	}
	x=a_fptr.p_fptr(1,x);
}

// 25 : malloc
// 31 : minus
// 37 : plus