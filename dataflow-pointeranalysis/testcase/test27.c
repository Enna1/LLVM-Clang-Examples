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
int foo(int a, int b,struct fptr * c_fptr) {
   return c_fptr->p_fptr(a, b);
}

int clever(int a, int b, struct fptr * c_fptr) {
	  c_fptr->p_fptr=plus;
    return foo(a, b, c_fptr);
}


int moo(char x, int op1, int op2) {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;
    int (*t_fptr)(int, int) = 0;
    struct fptr m_fptr;
    m_fptr.p_fptr=NULL;
    if (x == '+') {
       t_fptr = a_fptr;
    } 
    else if (x == '-') {
       t_fptr = s_fptr;
    }
    m_fptr.p_fptr=t_fptr;
    unsigned result = clever(op1, op2, &m_fptr);
    return 0;
}

// 22 : plus
// 27 : foo
// 44 : clever