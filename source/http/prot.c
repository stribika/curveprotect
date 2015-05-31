/* Public domain. */

#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include "prot.h"

int prot_gid(int g) {

    gid_t gid = g;

    if (setgroups(1, &gid) == -1) return -1;
    return setgid(gid);
}

int prot_uid(int u) {

    uid_t uid = u;

    return setuid(uid);
}
