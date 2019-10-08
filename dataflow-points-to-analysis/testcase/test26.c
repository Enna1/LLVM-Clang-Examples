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
void make_alias(struct wfsptr *a,struct fsptr *b)
{
	a->wfptr->sptr->p_fptr=b->sptr->p_fptr;
}
void foo(int x)
{
	struct fptr a_fptr;
	struct fptr b_fptr;
	struct fsptr s_fptr;
	struct fsptr m_fptr;
	struct wfsptr* w_fptr=(struct wfsptr*)malloc(sizeof(struct wfsptr));
	s_fptr.sptr=&a_fptr;
	m_fptr.sptr=&b_fptr;
	b_fptr.p_fptr=minus;
	w_fptr->wfptr=&s_fptr;
	if(x>1)
	{
		 a_fptr.p_fptr=plus;
		 x=s_fptr.sptr->p_fptr(1,x);
		 make_alias(w_fptr,&m_fptr);
	}else
	{
		w_fptr->wfptr->sptr->p_fptr=minus;
	}
	x=a_fptr.p_fptr(1,x);
}

// 31 : malloc
// 39 : plus
// 40 : make_alias
// 45 : minus