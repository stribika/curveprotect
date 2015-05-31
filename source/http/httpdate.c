#include "caltime.h"
#include "httpdate.h"

static char weekday[7][7] = {
  " Sun, ", " Mon, ", " Tue, ", " Wed, ", " Thu, ", " Fri, ", " Sat, "
} ;

static char month[12][6] = {
  " Jan ", " Feb ", " Mar ", " Apr ", " May ", " Jun "
, " Jul ", " Aug ", " Sep ", " Oct ", " Nov ", " Dec "
} ;

int httpdate(stralloc *sa,struct tai *t)
{
  struct caltime ct;
  int i;

  caltime_utc(&ct,t,&i,(int *) 0);

  if (i < 0) i = 0;
  if (i > 6) i = 6;
  if (!stralloc_copys(sa,weekday[i])) return 0;
  if (!stralloc_catuint0(sa,ct.date.day,2)) return 0;
  i = ct.date.month - 1;
  if (i < 0) i = 0;
  if (i > 11) i = 11;
  if (!stralloc_cats(sa,month[i])) return 0;
  if (!stralloc_catuint0(sa,ct.date.year,0)) return 0;
  if (!stralloc_cats(sa," ")) return 0;
  if (!stralloc_catuint0(sa,ct.hour,2)) return 0;
  if (!stralloc_cats(sa,":")) return 0;
  if (!stralloc_catuint0(sa,ct.minute,2)) return 0;
  if (!stralloc_cats(sa,":")) return 0;
  if (!stralloc_catuint0(sa,ct.second,2)) return 0;
  if (!stralloc_cats(sa," GMT")) return 0;

  return 1;
}
