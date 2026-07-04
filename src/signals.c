/*
 * signals.c — Signal handling for Ctrl-C, Ctrl-Z (Part E.3).
 *
 * The shell and its foreground children share the same process group.
 * When Ctrl-C/Z is pressed, the terminal sends SIGINT/SIGTSTP to the
 * entire group.  Children have SIG_DFL and respond normally (die/stop).
 * The shell's handlers just absorb the signal so the shell itself
 * doesn't die or stop.
 */

#include <signal.h>
#include <unistd.h>

#include "signals.h"

static volatile sig_atomic_t fg_pid = -1;

void set_fg_pid(pid_t pid) { fg_pid = (sig_atomic_t)pid; }
pid_t get_fg_pid(void)     { return (pid_t)fg_pid; }

static void sigint_handler(int sig)
{
    (void)sig;
    /* Shell absorbs SIGINT — child (SIG_DFL) gets it from the terminal */
}

static void sigtstp_handler(int sig)
{
    (void)sig;
    /* Shell absorbs SIGTSTP — child (SIG_DFL) gets it from the terminal */
}

void setup_signal_handlers(void)
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);

    /* SIGINT: use SA_RESTART so getline etc. restart after Ctrl-C
       when no foreground child is running */
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    /* SIGTSTP: NO SA_RESTART so waitpid can return after child stops */
    sa.sa_flags = 0;
    sa.sa_handler = sigtstp_handler;
    sigaction(SIGTSTP, &sa, NULL);
}
