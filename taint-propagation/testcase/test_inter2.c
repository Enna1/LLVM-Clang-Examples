#include <stdio.h>
#include <stdlib.h>

void someFunction(int *i, char* buffer)
{
	*i = buffer[1];
}

int someFunction2(int j)
{
	return j+1;
}

int main(int argc, char **argv)
{
	FILE* inf = fopen(argv[1], "r"); // inf
	fseek(inf, 0, SEEK_END);
	long size = ftell(inf);
	rewind(inf);
	char* buffer = malloc(size+1);
	fread(buffer, size, 1, inf);
	buffer[size] = '\0';
	fclose(inf);
	int x = buffer[1];
	int array[5];
	array[1] = buffer[3];
	int y;
	someFunction(&y, buffer);
	int ret = someFunction2(x);
	free(buffer);
	return ret;
}
