#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <alloca.h>

int main(int argc, char argv[])
{
	int onStack;
	int *onHeap=(int *)malloc(sizeof(int));
	printf("Starting Stack at %x \n Starting Heap at %x \n",&onStack,onHeap);
	free(onHeap);
	return 0;
}

