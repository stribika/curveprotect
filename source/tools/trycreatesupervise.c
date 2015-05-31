/*
20120524
Jan Mojzis
Public domain.
*/

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

int main(int argc, char **argv) {

    mkdir("supervise", 0700);
    mkfifo("supervise/control", 0600);
    mkfifo("supervise/ok", 0600);
    _exit(0);
}

