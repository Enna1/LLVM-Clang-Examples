#include <stdlib.h>
int plus(int a, int b) {
   return a+b;
}

int minus(int a, int b) {
   return a-b;
}
void foo(int x)
{
	int (**a_fptr)(int,int)=(int (**)(int,int))malloc(sizeof(int (*)(int,int)));
	if(x>1)
	{
		*a_fptr=plus;
		 x=(*a_fptr)(1,x);
		 *a_fptr=minus;
	}else
	{
		*a_fptr=minus;
	}
	x=(*a_fptr)(1,x);
}
// 11 : malloc
// 15 : plus
// 21 : minus