#include <sys/time.h>
#include <minix/com.h>
#include <minix/type.h>
#include <minix/syslib.h>
#include <stdio.h>
#include <math.h>
/*#include <time.h>*/
#include <unistd.h>

#define I386_PAGE_SIZE		4096
#define VM_PAGE_SIZE	I386_PAGE_SIZE

static vm_p2_stats g_ms;



int main(int argc, char **argv){
	/*fprintf(stdout,"sys_p2_echo() returned %d\n",sys_p2_echo());
	sys_p2_get_stat(&g_ms);
	fprintf(stdout,"%d\t%d\t%d\t%f\t%f\n",g_ms.nodes,g_ms.pages,g_ms.largest,g_ms.avg_pages,sqrt(g_ms.var_pages));*/
	int beep = 0;

	if(argc!=3){
		fprintf(stdout,"argc!=3\n");
		return 1;
	}

	if(argv[1][0]=='/'){
		++argv[1];
		beep = 1;
	}

	{
#define TRY(S,I) if(!strcmp(argv[2],(S))) { sys_p2_set_policy(I); }
		TRY("first",VM_P2_FRST_FIT) else
		TRY("next",VM_P2_NEXT_FIT) else
		TRY("best",VM_P2_BEST_FIT) else
		TRY("worst",VM_P2_WRST_FIT) else
		TRY("random",VM_P2_RAND_FIT)
#undef TRY
	}

	{
		FILE *f = fopen(argv[1],"w");
		int ticker = 0;
		const float coeff = (VM_PAGE_SIZE/1048576.0f);
		struct timeval next_stop;
		gettimeofday(&next_stop,0);
		while(1){
			struct timeval tv;
			sys_p2_get_stat(&g_ms);
			if(beep) fprintf(stdout,"Beep %d\n",ticker);
			fprintf(f,"%d\t%d\t%.2f\t%.2f\n",ticker,g_ms.pages,g_ms.avg_pages*coeff,sqrt(g_ms.var_pages)*coeff);
			fflush(f); 
			++ticker;
			++next_stop.tv_sec;
			gettimeofday(&tv,0);
			/*fprintf(stdout,"%d, %d ; %d, %d\n",tv.tv_sec,tv.tv_usec,next_stop.tv_sec,next_stop.tv_usec);*/
			usleep(next_stop.tv_usec+(next_stop.tv_sec-tv.tv_sec)*1000000-tv.tv_usec);
		}
		fclose(f);
	}
	return 0;
}
