#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define	MAXFORKS	300
#define	MAXSTARTTIME	300
#define	MAXRUNTIME	200

static const char 	*memuseprog = "memuse";
static char		*progname;
static int memsizes[] = {1, 3, 6, 8, 12, 16, 24, 32, 40};

struct memuse {
  int	mem;
  int	start;
  int	run;
};

void
printUsageAndExit ()
{
  fprintf (stderr,"Usage: %s <usage0+start0+time0> [usage1+start1+time1] ...\n",
	   progname);
  fprintf (stderr, "\tusage is memory usage in MB (must correspond to memuse program).\n");
  fprintf (stderr, "\tstart is start time in seconds (0-%d).\n", MAXSTARTTIME);
  fprintf (stderr, "\ttime is time consumed in seconds (1-%d).\n", MAXRUNTIME);
  fprintf (stderr, "\tThere must be 1-%d usage+start+time triples.\n",
	   MAXFORKS);
  exit (-1);
}

int
main (int argc, char *argv[])
{
  int		i, j, best;
  struct memuse	x[MAXFORKS];
  struct memuse tmp;
  int		nforks = argc-1;
  int		prevtime;
  char		*memuseargs[3];
  char		buf[200];

  progname = argv[0];
  if (argc == 1 || argc >= 300) {
    printUsageAndExit ();
  }

  for (i = 0; i < nforks; i++) {
    if (sscanf (argv[i+1], "%d+%d+%d",
		&(x[i].mem), &(x[i].start), &(x[i].run)) != 3) {
      fprintf (stderr, "%s: specifier %d (%s) malformed - exiting!\n",
	       progname, i, argv[i+1]);
      printUsageAndExit ();
    }
    if (x[i].start < 0 || x[i].start > MAXSTARTTIME ||
	x[i].run < 1 || x[i].run > MAXRUNTIME) {
      fprintf (stderr, "%s: program %d has a bad time (start=%d, runtime=%d)\n",
	       progname, i, x[i].start, x[i].run);
      printUsageAndExit ();
    }
    for (j = 0; j < sizeof (memsizes) / sizeof (memsizes[0]); j++) {
      if (x[i].mem == memsizes[j]) {
	break;
      }
    }
    if (j == sizeof (memsizes) / sizeof (memsizes[0])) {
      fprintf (stderr, "%s: program %d has a bad size (%d)\n",
	       progname, i, x[i].mem);
      printUsageAndExit ();
    }
  }

  /* We need to sort the programs and then run them.  It might seem easier
     to simply run them and sleep first in the fork, but that would create
     too many processes.  Instead, we're going to sort the programs by
     start time.  Selection sort is nice and quick, so we'll use it.
   */

  for (i = 0; i < nforks-1; i++) {
    for (best = i, j = i+1; j < nforks; j++) {
      if (x[j].start < x[best].start) {
	best = j;
      }
    }
    if (best != i) {
      tmp = x[i];
      x[i] = x[best];
      x[best] = tmp;
    }
  }

  prevtime = 0;
  for (i = 0; i < nforks; i++) {
    if (x[i].start > prevtime) {
      sleep (x[i].start - prevtime);
      prevtime = x[i].start;
    }
    printf ("Running %d MB at t=%d for %d seconds.\n", x[i].mem, x[i].start,
	    x[i].run);
    /* Child runs the other process */
    if (fork () == 0) {
      memuseargs[0] = buf;
      memuseargs[1] = buf+150;
      sprintf (memuseargs[0], "%s%d", memuseprog, x[i].mem);
      sprintf (memuseargs[1], "%d", x[i].run);
      memuseargs[2] = NULL;
      execv (memuseargs[0], memuseargs);
    }
  }
  sleep (x[nforks-1].run+1);
}
