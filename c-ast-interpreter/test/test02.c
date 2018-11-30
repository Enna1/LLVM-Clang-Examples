extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a;
   int b;
   a = 10;
   b = 10;
   PRINT(a+b);
}
