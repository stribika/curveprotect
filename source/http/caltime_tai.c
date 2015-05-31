#include "tai.h"
#include "leapsecs.h"
#include "caldate.h"
#include "caltime.h"

/* XXX: breaks tai encapsulation */

void caltime_tai(ct,t)
struct caltime *ct;
struct tai *t;
{
  long day;
  long s;

  /* XXX: check for overflow? */

  day = caldate_mjd(&ct->date);

  s = ct->hour * 60 + ct->minute;
  s = (s - ct->offset) * 60 + ct->second;

  t->x = day * 86400ULL + 4611686014920671114ULL + (long long) s;

  leapsecs_add(t,ct->second == 60);
}

