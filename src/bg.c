/*
 * bg.c — Background job tracking and process management.
 *
 * D.2: bg_add_job / bg_check_jobs — track and poll background PIDs.
 * E.1: builtin_activities         — list running/stopped processes.
 * E.2: builtin_ping               — send a signal to a process.
 */

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "types.h"
#include "bg.h"

typedef struct {
    pid_t pid;
    char *cmd_name;
    int   job_number;
    int   active;
    int   stopped;     /* 1 if process is currently stopped */
} BGJob;

static BGJob jobs[MAX_BG_JOBS];
static int   job_count    = 0;
static int   next_job_num = 1;

/* ------------------------------------------------------------------ */
/*  D.2 — Background job tracking                                    */
/* ------------------------------------------------------------------ */

void bg_add_job(pid_t pid, const char *cmd_name)
{
    if (job_count >= MAX_BG_JOBS) return;

    jobs[job_count].pid        = pid;
    jobs[job_count].cmd_name   = strdup(cmd_name);
    jobs[job_count].job_number = next_job_num++;
    jobs[job_count].active     = 1;
    jobs[job_count].stopped    = 0;

    printf("[%d] %d\n", jobs[job_count].job_number, (int)pid);
    fflush(stdout);

    job_count++;
}

void bg_check_jobs(void)
{
    for (int i = 0; i < job_count; i++) {
        if (!jobs[i].active)
            continue;

        int status;
        pid_t result = waitpid(jobs[i].pid, &status,
                               WNOHANG | WUNTRACED | WCONTINUED);

        if (result <= 0)
            continue;

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            /* Process terminated */
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
        } else if (WIFSTOPPED(status)) {
            jobs[i].stopped = 1;
        } else if (WIFCONTINUED(status)) {
            jobs[i].stopped = 0;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  E.1 — activities                                                  */
/* ------------------------------------------------------------------ */

/* Comparison for qsort: lexicographic by command name. */
typedef struct { pid_t pid; const char *name; int stopped; } ActiveEntry;

static int cmp_active(const void *a, const void *b)
{
    return strcmp(((const ActiveEntry *)a)->name,
                 ((const ActiveEntry *)b)->name);
}

void builtin_activities(void)
{
    /* Collect active jobs, pruning any that have silently exited */
    ActiveEntry active[MAX_BG_JOBS];
    int n = 0;

    for (int i = 0; i < job_count; i++) {
        if (!jobs[i].active)
            continue;

        /* Double-check: is the process still alive? */
        if (kill(jobs[i].pid, 0) != 0) {
            free(jobs[i].cmd_name);
            jobs[i].cmd_name = NULL;
            jobs[i].active   = 0;
            continue;
        }

        active[n].pid     = jobs[i].pid;
        active[n].name    = jobs[i].cmd_name;
        active[n].stopped = jobs[i].stopped;
        n++;
    }

    /* Sort lexicographically by command name */
    if (n > 1)
        qsort(active, (size_t)n, sizeof(ActiveEntry), cmp_active);

    /* Print */
    for (int i = 0; i < n; i++) {
        printf("[%d] : %s - %s\n", (int)active[i].pid,
               active[i].name,
               active[i].stopped ? "Stopped" : "Running");
    }
    fflush(stdout);
}

/* ------------------------------------------------------------------ */
/*  E.2 — ping                                                        */
/* ------------------------------------------------------------------ */

void builtin_ping(int argc, char **argv)
{
    if (argc != 3) {
        printf("Invalid syntax!\n");
        return;
    }

    /* Validate pid is a number */
    for (int i = 0; argv[1][i] != '\0'; i++) {
        if (!isdigit((unsigned char)argv[1][i])) {
            printf("Invalid syntax!\n");
            return;
        }
    }

    /* Validate signal_number is a number */
    for (int i = 0; argv[2][i] != '\0'; i++) {
        if (!isdigit((unsigned char)argv[2][i])) {
            printf("Invalid syntax!\n");
            return;
        }
    }

    pid_t pid    = (pid_t)atoi(argv[1]);
    int   signum = atoi(argv[2]);
    int   actual = signum % 32;

    if (kill(pid, actual) != 0) {
        printf("No such process found\n");
    } else {
        printf("Sent signal %d to process with pid %d\n",
               signum, (int)pid);
    }
    fflush(stdout);
}
