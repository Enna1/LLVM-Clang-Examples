#include <stdio.h>
#include <stdlib.h>

int x;

int main(int argc, char **argv)
{
	FILE* inf = fopen(argv[1], "r");
	fseek(inf, 0, SEEK_END);
	long size = ftell(inf);
	rewind(inf);
	char* buffer = malloc(size+1);
	fread(buffer, size, 1, inf);
	buffer[size] = '\0';
	fclose(inf);
	x = buffer[1];
	free(buffer);
	return x;
}
