#include <lib.h>
#include <string.h>
#include <unistd.h>

int setbid(int bid) /* so_2021 */
{
  message m;
  memset(&m, 0, sizeof(m));

  m.m_m1.m1i1 = bid;
  if (_syscall(PM_PROC_NR, PM_SETBID, &m) < 0) {
    return -1;
  }

  return 0;
}
