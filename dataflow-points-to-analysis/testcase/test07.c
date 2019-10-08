#include <stdlib.h>
struct fptr
{
	int (*p_fptr)(int, int);
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
	if(x>1)
	{
		a_fptr.p_fptr=plus;
		 x=a_fptr.p_fptr(1,x);
		 a_fptr.p_fptr=minus;
	}else
	{
		a_fptr.p_fptr=minus;
	}
	x=a_fptr.p_fptr(1,x);
}

// 19 : plus
// 25 : minus