/*
 * bg.h — Background job tracking and process management.
 *
 * Parts D.2 (background execution), E.1 (activities), E.2 (ping).
 */

#ifndef BG_H
#define BG_H

#include <sys/types.h>

/* Add a background job. Prints [job_number] pid. */
void bg_add_job(pid_t pid, const char *cmd_name);

/* Check for completed/stopped background jobs. Print exit status. */
void bg_check_jobs(void);

/* E.1: List all running/stopped background processes. */
void builtin_activities(void);

/* E.2: Send a signal to a process. */
void builtin_ping(int argc, char **argv);

#endif /* BG_H */
