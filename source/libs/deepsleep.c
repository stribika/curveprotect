#include <poll.h>
#include "milliseconds.h"
#include "deepsleep.h"

void deepsleep(long long t) {

    long long deadline, tm;
    struct pollfd p;

    if (t <= 0) return;

    deadline = milliseconds() + 1000 * t;

    for (;;) {
        tm = deadline - milliseconds();
        if (tm <= 0) break;
        if (tm > 1000000) tm = 1000000;
        poll(&p, 0, tm);
    }
    return;
}
