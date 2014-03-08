#include <lib.h>
#define fchmod	_fchmod
#include <sys/stat.h>

PUBLIC int fchmod(int fd, mode_t mode)
{
  message m;

  m.m1_i1 = fd;
  m.m1_i2 = mode;
  return(_syscall(FS, FCHMOD, &m));
}
