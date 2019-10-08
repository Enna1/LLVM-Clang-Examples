#include <stdio.h>
#include <stdlib.h>

void someFunction(char *i, char* buffer, int *j)
{
	*i = buffer[1];
	*j = *i;
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
	char y;
	int ret = 0;
	someFunction(&y, buffer, &ret);
	free(buffer);
	return ret;
}
