#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
int main(int argc,char **argv){ 
	int d0 = open(argv[1],0);
	int d1 = open(argv[1],0);
	assert(d0>0);
	assert(d1>0);
	sleep(60);
	return 0; 
}

