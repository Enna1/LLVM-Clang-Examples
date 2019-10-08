#include <stdio.h>
#include <stdlib.h>

struct A
{
    int *x;
    int *y;
};

int main(int argc, char** argv)
{
    struct A arr[10];
    char buf;
    FILE* inf = fopen(argv[1], "r");
    fread(&buf, sizeof(buf), 1, inf);
    int bufi = buf;
    arr[argc].x = &bufi;
    arr[argc].y = &bufi;
    return *arr[argc].y;
}
