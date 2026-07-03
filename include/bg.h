/*
 * bg.h — Background job tracking and process management.
 *
 * Parts D.2, E.1, E.2, E.3, E.4.
 */

#ifndef BG_H
#define BG_H

#include <sys/types.h>

/* D.2: Add a background job. Prints [job_number] pid. */
void bg_add_job(pid_t pid, const char *cmd_name);

/* D.2: Check for completed/stopped background jobs. */
void bg_check_jobs(void);

/* E.1: List all running/stopped background processes. */
void builtin_activities(void);

/* E.2: Send a signal to a process. */
void builtin_ping(int argc, char **argv);

/* E.3: Add a stopped foreground process to background list. */
void bg_add_stopped_job(pid_t pid, const char *cmd_name);

/* E.3: Kill all active background processes (Ctrl-D cleanup). */
void bg_kill_all(void);

/* E.4: Bring a job to foreground. */
void builtin_fg(int argc, char **argv);

/* E.4: Resume a stopped job in background. */
void builtin_bg(int argc, char **argv);

#endif /* BG_H */
