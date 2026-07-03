/*
 * signals.h — Signal handling for Ctrl-C, Ctrl-Z (Part E.3).
 */

#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>

/* Install SIGINT and SIGTSTP handlers. */
void setup_signal_handlers(void);

/* Track the current foreground child process group. */
void set_fg_pid(pid_t pid);
pid_t get_fg_pid(void);

#endif /* SIGNALS_H */
