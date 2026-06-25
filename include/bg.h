/*
 * bg.h — Background job tracking (Part D.2).
 */

#ifndef BG_H
#define BG_H

#include <sys/types.h>

/* Add a background job. Prints [job_number] pid. */
void bg_add_job(pid_t pid, const char *cmd_name);

/* Check for completed background jobs. Print exit status. */
void bg_check_jobs(void);

#endif /* BG_H */
