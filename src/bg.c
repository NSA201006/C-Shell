/*
 * bg.c — Background job tracking (Part D.2).
 *
 * Maintains a list of background PIDs and their command names.
 * bg_check_jobs() uses waitpid(WNOHANG) to poll for completion.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "bg.h"

#define MAX_BG_JOBS 256

typedef struct {
    pid_t pid;
    char *cmd_name;
    int   job_number;
    int   active;
} BGJob;

static BGJob jobs[MAX_BG_JOBS];
static int   job_count    = 0;
static int   next_job_num = 1;

void bg_add_job(pid_t pid, const char *cmd_name)
{
    if (job_count >= MAX_BG_JOBS)
        return;

    jobs[job_count].pid        = pid;
    jobs[job_count].cmd_name   = strdup(cmd_name);
    jobs[job_count].job_number = next_job_num++;
    jobs[job_count].active     = 1;

    printf("[%d] %d\n", jobs[job_count].job_number, (int)pid);
    fflush(stdout);

    job_count++;
}

void bg_check_jobs(void)
{
    for (int i = 0; i < job_count; i++) {
        if (!jobs[i].active)
            continue;

        int   status;
        pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);

        if (result == jobs[i].pid) {
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                printf("%s with pid %d exited normally\n",
                       jobs[i].cmd_name, (int)jobs[i].pid);
            } else {
                printf("%s with pid %d exited abnormally\n",
                       jobs[i].cmd_name, (int)jobs[i].pid);
            }
            fflush(stdout);
            free(jobs[i].cmd_name);
            jobs[i].cmd_name = NULL;
            jobs[i].active   = 0;
        }
    }
}
