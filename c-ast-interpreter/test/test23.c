extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);


void swap(int *a, int *b) {
   int temp;
   temp = *a;
   *a = *b;
   *b = temp;
}

int main() {
   int* a; 
   int* b;
   a = (int *)MALLOC(sizeof(int));
   b = (int *)MALLOC(sizeof(int *));
   
   *b = 24;
   *a = 42;

   swap(a, b);

   PRINT(*a);
   PRINT(*b);
   FREE(a);
   FREE(b);
   return 0;
}

