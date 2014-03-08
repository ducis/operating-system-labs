#include <stdio.h>
#include <unistd.h>

int main(){
    while(!feof(stdin)){
		int ch = fgetc(stdin);
		printf("zzz says %d\n",ch);
		if(ch==-1 && !feof(stdin)){
			printf("interrupted\n");
			return;
		}
    }
	printf("zzz DIEEEEES\n");
    return 0;
}
