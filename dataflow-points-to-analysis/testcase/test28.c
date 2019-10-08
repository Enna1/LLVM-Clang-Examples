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
int foo(int a, int b, int (**a_fptr)(int, int), int(**b_fptr)(int, int)) {
   return (*b_fptr)(a, b);
}

int clever(int a, int b, int (**a_fptr)(int, int), int(**b_fptr)(int, int)) {
	int (*t_fptr)(int, int)=*a_fptr;
	*a_fptr=*b_fptr;
	*b_fptr=t_fptr;
    return foo(a, b, a_fptr, b_fptr);
}


int moo(char x, int op1, int op2) {
	int (**a_fptr)(int, int) = (int (**)(int, int))malloc(sizeof(int (*)(int, int)));
    *a_fptr= plus;
    int (**s_fptr)(int, int) = (int (**)(int, int))malloc(sizeof(int (*)(int, int)));
    *s_fptr = minus;
    int (**t_fptr)(int, int) = (int (**)(int, int))malloc(sizeof(int (*)(int, int)));

    if (x == '+') {
       t_fptr = a_fptr;
    } 
    else if (x == '-') {
       t_fptr = s_fptr;
    }

    unsigned result = clever(op1, op2, a_fptr, t_fptr);
    return 0;
}

// 22 : plus
// 29 : foo
// 34 : malloc
// 36 : malloc
// 38 : malloc
// 47 : clever
