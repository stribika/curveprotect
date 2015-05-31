#include "sig.h"
#include <signal.h>

#define X(e,s) if (i == e) return s;

const char *sig_str(int i)
{

    X(sig_alarm,"SIGALRM (alarm clock)")
    X(sig_child,"SIGCHLD (child status has change)")
    X(sig_cont,"SIGCONT (continue)")
    X(sig_hangup,"SIGHUP (hangup)")
    X(sig_int,"SIGINT (interrupt)")
    X(sig_pipe,"SIGPIPE (broken pipe)")
    X(sig_term,"SIGTERM (termination)")

#ifdef SIGQUIT 
    X(SIGQUIT,"SIGQUIT (quit)")
#endif
#ifdef SIGILL
    X(SIGILL,"SIGILL (illegal instruction)")
#endif
#ifdef SIGTRAP
    X(SIGTRAP,"SIGTRAP (trace trap)")
#endif
#ifdef SIGIOT
    X(SIGIOT,"SIGIOT (IOT trap)")
#endif
#ifdef SIGABRT
    X(SIGABRT,"SIGABRT (abort)")
#endif
#ifdef SIGEMT
    X(SIGEMT,"SIGEMT (EMT instruction)")
#endif
#ifdef SIGFPE
    X(SIGFPE,"SIGFPE (floating-point exception)")
#endif
#ifdef SIGKILL
    X(SIGKILL,"SIGKILL (kill, unblockable)")
#endif
#ifdef SIGBUS
    X(SIGBUS,"SIGBUS (bus error)")
#endif
#ifdef SIGSEGV
    X(SIGSEGV,"SIGSEGV (segment violation)")
#endif
#ifdef SIGSYS
    X(SIGSYS,"SIGSYS (bad system call)")
#endif
#ifdef SIGUSR1
    X(SIGUSR1,"SIGUSR1 (user defined signal 1)")
#endif
#ifdef SIGUSR2
    X(SIGUSR2,"SIGUSR2 (user defined signal 2)")
#endif
#ifdef SIGCLD
    X(SIGCLD,"SIGCLD (child status has change)")
#endif
#ifdef SIGPWR
    X(SIGPWR,"SIGPWR (power failure restart)")
#endif
#ifdef SIGWINCH
    X(SIGWINCH,"SIGWINCH (window size change)")
#endif
#ifdef SIGURG
    X(SIGURG,"SIGURG (urgent socket condition)")
#endif
#ifdef SIGPOLL
    X(SIGPOLL,"SIGPOLL (pollable event occurred)")
#endif
#ifdef SIGIO
    X(SIGIO,"SIGIO (i/o now possible)")
#endif
#ifdef SIGSTOP
    X(SIGSTOP,"SIGSTOP (stop, unblockable)")
#endif
#ifdef SIGTSTP
    X(SIGTSTP,"SIGTSTP (keyboard stop)")
#endif
#ifdef SIGTTIN
    X(SIGTTIN,"SIGTTIN (background read from tty)")
#endif
#ifdef SIGTTOU
    X(SIGTTOU,"SIGTTOU (background write to tty)")
#endif
#ifdef SIGVTALRM
    X(SIGVTALRM,"SIGVTALRM (virtual alarm clock)")
#endif
#ifdef SIGPROF
    X(SIGPROF,"SIGPROF (profiling alarm clock)")
#endif
#ifdef SIGXCPU
    X(SIGXCPU,"SIGXCPU (exceeded cpu limit)")
#endif
#ifdef SIGXFSZ
    X(SIGXFSZ,"SIGXFSZ (exceeded file size limit)")
#endif

/* ??? */
#ifdef SIGWAITING
    X(SIGWAITING,"SIGWAITING (reserved signal no longer used by threading code)")
#endif
#ifdef SIGLWP
    X(SIGLWP,"SIGLWP (reserved signal no longer used by threading code)")
#endif
#ifdef SIGFREEZE
    X(SIGFREEZE,"SIGFREEZE (special signal used by CPR)")
#endif
#ifdef SIGTHAW
    X(SIGTHAW,"SIGTHAW (special signal used by CPR)")
#endif
#ifdef SIGCANCEL
    X(SIGCANCEL,"SIGCANCEL (reserved signal for thread cancellation)")
#endif
#ifdef SIGLOST
    X(SIGLOST,"SIGLOST (resource lost)")
#endif
#ifdef SIGXRES
    X(SIGXRES,"SIGXRES (resource control exceeded)")
#endif
#ifdef SIGJVM1
    X(SIGJVM1,"SIGJVM1 (reserved signal for Java Virtual Machine)")
#endif
#ifdef SIGJVM2
    X(SIGJVM2,"SIGJVM2 (reserved signal for Java Virtual Machine)")
#endif
#ifdef SIGSTKFLT
    X(SIGSTKFLT,"SIGSTKFLT (stack fault)")
#endif

    return "unknown signal";
}
