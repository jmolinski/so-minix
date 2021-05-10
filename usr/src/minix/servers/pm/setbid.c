#include "pm.h"  // TODO SO2137 this first
#include "mproc.h"

int do_setbid(void) /* so_2021 */
{
  struct mproc *rmp = mp;

  message mess;
  mess.m_pm_sched_scheduling_setbid.endpoint = rmp->mp_endpoint;
  mess.m_pm_sched_scheduling_setbid.bid = m_in.m_m1.m1i1;

  return _taskcall(rmp->mp_scheduler, SCHEDULING_SETBID, &mess);
}
