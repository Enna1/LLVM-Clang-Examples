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
void foo(int x)
{
	struct fptr a_fptr;
	struct fsptr s_fptr;
	s_fptr.sptr=&a_fptr;
	if(x>1)
	{
		 a_fptr.p_fptr=plus;
		 x=s_fptr.sptr->p_fptr(1,x);
		 a_fptr.p_fptr=minus;
	}else
	{
		s_fptr.sptr->p_fptr=minus;
	}
	x=a_fptr.p_fptr(1,x);
}

//25 : plus
//31 : minus