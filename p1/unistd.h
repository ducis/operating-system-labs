/*fake posix calls*/
#include <errno.h>
#include <io.h> /*windows _dup2*/

pid_t fork(void){
    static int i=0;
    i = (i+1)%32;
    printf("fork() returns %d\n",i);
	return i;
}

int execvp(const char *file, char *const argv[]){
    return 0;
}

