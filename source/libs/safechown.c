#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "direntry.h"
#include "open.h"
#include "str.h"
#include "e.h"
#include "safechown.h"

static int recursivechowncwd(long long uid, long long gid);
static int recursivechowndir(const char *d, long long uid, long long gid);

int recursivechowncwd(long long uid, long long gid) {

    direntry *d;
    DIR *dir;
    struct stat st;

    dir = opendir(".");
    if (!dir) return -1;
    for (;;) {
        errno = 0;
        d = readdir(dir);
        if (!d) break;
        if (str_equal(d->d_name, ".")) continue;
        if (str_equal(d->d_name, "..")) continue;

        if (lstat(d->d_name, &st) == -1) break;
        if (S_ISLNK(st.st_mode)) continue; /* ignore symbolic links */
        if (!S_ISDIR(st.st_mode)) if (chown(d->d_name, uid, gid) == 0) continue;
        if (recursivechowndir(d->d_name, uid, gid) == -1) break;
    }
    if (errno) {
        closedir(dir);
        return -1;
    }
    closedir(dir);
    return 0;
}

int recursivechowndir(const char *d, long long uid, long long gid) {

    long long basefd, r;

    basefd = open_cwd();
    if (basefd == -1) return -1;
    if (chdir(d) == -1) { close(basefd); return -1; }
    r = recursivechowncwd(uid, gid);
    if (fchdir(basefd) == -1) r = -1;
    close(basefd);
    if (r != 0) return -1;
    return chown(d, uid, gid);
}


int safechown(const char *d0, long long uid, long long gid, int flagrecursive) {

    struct stat st;
    long long pid;
    int childstatus;

    if (lstat(d0, &st) == -1) return -1;
    if (S_ISLNK(st.st_mode)) {errno = EINVAL; return -1;} /* reject symbolic links */
    if (!S_ISDIR(st.st_mode)) return chown(d0, uid, gid);
    if (!flagrecursive) return chown(d0, uid, gid);

    if (geteuid() != 0) { errno = EPERM; return -1; }

    switch(pid = fork()) {
        case -1:
            return -1;
        case 0:
            if (chown(d0, uid, gid) == -1) _exit(errno);
            if (chdir(d0) == -1) _exit(errno);
            if (chroot(".") == -1) _exit(errno);
            if (recursivechowncwd(uid, gid) == -1) _exit(errno);
            _exit(0);
    }
    while (waitpid(pid, &childstatus, 0) != pid) {};
    if (!WIFEXITED(childstatus)) {errno = -WTERMSIG(childstatus); return -1;} /* XXX ... unknown error */
    errno = WEXITSTATUS(childstatus);
    if (errno) return -1;
    return 0;
}
