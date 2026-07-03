/*
 * signals.c — Signal handling for Ctrl-C, Ctrl-Z (Part E.3).
 *
 * SIGINT  handler: forwards SIGINT  to foreground child process group.
 * SIGTSTP handler: forwards SIGTSTP to foreground child process group.
 * Shell itself survives both signals.
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
    if (fg_pid > 0)
        kill(-fg_pid, SIGINT);   /* send to process group */
}

static void sigtstp_handler(int sig)
{
    (void)sig;
    if (fg_pid > 0)
        kill(-fg_pid, SIGTSTP);  /* send to process group */
}

void setup_signal_handlers(void)
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = sigtstp_handler;
    sigaction(SIGTSTP, &sa, NULL);
}
