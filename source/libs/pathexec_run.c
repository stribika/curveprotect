#include <unistd.h>
#include "alloc.h"
#include "str.h"
#include "env.h"
#include "e.h"
#include "byte.h"
#include "pathexec.h"


void pathexec_run(char *file, char **argv, char **envp) {

    char *path;
    long long split, filelen;
    int savederrno;
    char *tmp, *p;

    if (file[str_chr(file,'/')]) {
        execve(file, argv, envp);
        return;
    }

    filelen = str_len(file) + 1;

    path = env_get("PATH");
    if (!path) path = "/bin:/usr/bin";

    savederrno = 0;
    for (;;) {
        split = str_chr(path,':');
        tmp = alloc(filelen + split + 2);
        if (!tmp) return;
        p = tmp;

        byte_copy(p, split, path); p += split;
        if (!split) *p++ = '.';
        *p++ = '/';
        byte_copy(p, filelen, file); p += filelen;

        execve(tmp, argv, envp);
        alloc_free(tmp);
        if (errno != ENOENT) {
            savederrno = errno;
            if ((errno != EACCES) && (errno != EPERM) && (errno != EISDIR)) return;
        }

        if (!path[split]) {
            if (savederrno) errno = savederrno;
            return;
        }
        path += split + 1;
    }
}
