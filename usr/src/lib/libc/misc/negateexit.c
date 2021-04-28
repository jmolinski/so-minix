#include <lib.h>
#include <minix/rs.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int negateexit(int negate) {
  message mess;
  memset(&mess, 0, sizeof(mess));

  mess.m_lc_pm_negateexit.negate = negate;

  if (_syscall(PM_PROC_NR, PM_NEGATEEXIT, &mess) < 0) {
    errno = ENOSYS;
    return -1;
  }

  return mess.m_lc_pm_negateexit.negate;
}