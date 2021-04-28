#include "mproc.h"
#include "pm.h"

int do_negateexit(void) {
  int old_status_negate_value = mp->status_negated;
  int negate = m_in.m_lc_pm_negateexit.negate;
  mp->status_negated = negate == 0 ? 0 : 1;
  mp->mp_reply.m_lc_pm_negateexit.negate = old_status_negate_value;

  return OK;
}
