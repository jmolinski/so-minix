#include <lib.h>
#include <string.h>
#include <unistd.h>

int setbid(int bid) /* so_2021 */
{
  message m;
  memset(&m, 0, sizeof(m));

  m.m_m1.m1i1 = bid;
  _syscall(PM_PROC_NR, PM_SETBID, &m);

  if (m.m_m1.m1i1 == -2) {
    errno = EINVAL;
    return -1;
  } else if (m.m_m1.m1i1 == -1) {
    errno = EPERM;
    return -1;
  } else {
    return 0;
  }
}
