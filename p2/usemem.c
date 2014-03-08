/*
 * $Id$
 */

#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#ifndef	SIZEINMB
#define	SIZEINMB	4
#endif

#define	NELEM		(SIZEINMB * 1024 * 1024/(sizeof (unsigned int)))

/* Forward definitions */
int rnd(void);		/* pseudo-random integer [0, M-1] */
void setseed(int);	/* set the random seed */
double uniform(void);	/* pseudo-random double [0, 1] */

static unsigned int mem[NELEM];

int
main (int argc, char *argv[])
{
  int	nsecs = 10;
  int	seed = 0;
  int	i, j;
  unsigned int	v;

  if (argc >= 2) {
    nsecs = atoi (argv[1]);
  }
  if (nsecs < 1) {
    nsecs = 1;
  } else if (nsecs > 200) {
    nsecs = 200;
  }

  /* Seed the RNG */
  if (argc >= 3) {
    seed = atoi (argv[2]);
    setseed (seed);
  } else {
    setseed (getpid () ^ time((void *)0));
  }

  for (i = 0; i < nsecs; i++) {
    for (j = 0; j < 200; j++) {
      mem[rnd() % NELEM] ^= rnd();
    }
    sleep (1);
  }
  for (v = 0, i = 0; i < NELEM; i += 512) {
    v ^= mem[i];
  }
  printf ("PID %d computed 0x%08x\n", getpid(), v);
  exit (0);
}


# define A 16807	/* known good value */
# define C 13
# define M 2147483647	/* 2^31 - 1 */
# define Q (M / A)
# define R (M % A)

# define SEED 67


static int _x = SEED;	/* call it _x to avoid conflicts with other uses of x */

/*  X(i+1) = a(X(i) % q) - r(X(i) / q) + m delta(Xi)
 *  delta(Xi) = X(i) / q - a X(i) % m
 * delta(Xi) = 1 if a(X(i) % q) - r(X(i) / q) < 0
 */

int rnd(void)
{
  _x = A * (_x % Q) - R * (_x / Q) + C;
  _x = _x < 0 ? _x + M : _x;

  assert(_x > 0 && _x < M);

  return (_x);
}

double uniform(void)
{
  double x = rnd() / (double) (M - 1);

  assert(x >= 0 && x <= 1);

  return (x);
}

void setseed (int v)
{
  _x = v;
  /* Call rnd() twice to get a better seed value - first one is often not
     so good.
  */
  rnd();
  rnd();
}
