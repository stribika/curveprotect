#include "httpdateparse.h"
#include "error.h"
#include "stralloc.h"
#include "caltime.h"
#include "scan.h"
#include "str.h"
#include "case.h"
#include "byte.h"

/*  "Mon, 26 Apr 2010 14:52:59 GMT" */


int httpdateparse(stralloc *tokens, struct tai *t){



    char *x;
    int state = 0;
    long u;
    unsigned int i;
    unsigned int len;
    int known = 0;

    struct caltime ct;

    byte_zero(&ct, sizeof (struct caltime));

    x = tokens->s;
    len = tokens->len;

    while(len > 0){


	if (*x == ' ' || *x == ':'){
	    ++x;--len;
	    continue;
	}

	i = scan_long(x,&u);
	switch(state) {
	    case 0:
		i = 4;
		if (case_startb(x,len,"Sun,")) break;
		if (case_startb(x,len,"Mon,")) break;
		if (case_startb(x,len,"Tue,")) break;
		if (case_startb(x,len,"Wed,")) break;
		if (case_startb(x,len,"Thu,")) break;
		if (case_startb(x,len,"Fri,")) break;
		if (case_startb(x,len,"Sat,")) break;
		return -1;
	    case 1:
		if (!i) return -1;
		ct.date.day = u;
		break;
	    case 2:
		i = 3;
		if (case_startb(x,len,"Jan")) { ct.date.month = 1;  break; }
		if (case_startb(x,len,"Feb")) { ct.date.month = 2;  break; }
		if (case_startb(x,len,"Mar")) { ct.date.month = 3;  break; }
		if (case_startb(x,len,"Apr")) { ct.date.month = 4;  break; }
		if (case_startb(x,len,"May")) { ct.date.month = 5;  break; }
		if (case_startb(x,len,"Jun")) { ct.date.month = 6;  break; }
		if (case_startb(x,len,"Jul")) { ct.date.month = 7;  break; }
		if (case_startb(x,len,"Aug")) { ct.date.month = 8;  break; }
		if (case_startb(x,len,"Sep")) { ct.date.month = 9;  break; }
		if (case_startb(x,len,"Oct")) { ct.date.month = 10; break; }
		if (case_startb(x,len,"Nov")) { ct.date.month = 11; break; }
		if (case_startb(x,len,"Dec")) { ct.date.month = 12; break; }
		return -1;
	    case 3:
		if (!i) return -1;
		if (u < 999) return -1;
		ct.date.year = u;
		break;
	    case 4:
		if (!i) return -1;
		ct.hour = u;
		break;
	    case 5:
		if (!i) return -1;
		ct.minute = u;
		break;
	    case 6:
		if (!i) return -1;
		ct.second = u;
		break;
	    case 7:
		i = 3;
		if (case_startb(x,len,"GMT")) { ct.offset = 0; known = 1;break; }
		return -1;
	    case 8:
		break;
	}
	x += i; len -= i;
	if (state < 8) ++state;
    }

    if (known) {
	caltime_tai(&ct,t);
	return 0;
    }
    return -1;
}


