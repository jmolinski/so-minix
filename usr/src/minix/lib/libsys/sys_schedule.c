#include "syslib.h"

int sys_schedule(endpoint_t proc_ep, /* so_2021 */
			int priority,
			int quantum,
			int cpu,
                        int bid)
{
	message m;

	m.m_lsys_krn_schedule.endpoint = proc_ep;
	m.m_lsys_krn_schedule.priority = priority;
	m.m_lsys_krn_schedule.quantum  = quantum;
	m.m_lsys_krn_schedule.cpu = cpu;
        m.m_lsys_krn_schedule.bid = bid;
	return(_kernel_call(SYS_SCHEDULE, &m));
}
