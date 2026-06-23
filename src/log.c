/*
 * log.c — Command history builtin (Part B.3).
 *
 * Manages a persistent list of up to MAX_LOG (15) commands,
 * stored in a file named LOG_FILE in the shell's home directory.
 *
 * Behaviours:
 *   log             — print history (oldest to newest)
 *   log purge       — clear history
 *   log execute <i> — execute the i-th command (1-indexed, newest first)
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "execute.h"
#include "parser.h"
#include "types.h"

/* ------------------------------------------------------------------ */
/*  In-memory history ring buffer                                     */
/* ------------------------------------------------------------------ */

static char *history[MAX_LOG];
static int   hist_count = 0;    /* number of entries currently stored */
static int   loaded     = 0;    /* lazy-load flag */

/* ------------------------------------------------------------------ */
/*  Persistence helpers                                               */
/* ------------------------------------------------------------------ */

/*
 * history_path — Build the full path to the history file.
 */
static void history_path(const char *shell_home, char *buf, size_t size)
{
    snprintf(buf, size, "%s/%s", shell_home, LOG_FILE);
}

/*
 * log_load — Read history entries from the persistent file.
 *            Called lazily on the first access.
 */
static void log_load(const char *shell_home)
{
    if (loaded)
        return;
    loaded = 1;

    char path[MAX_PATH_LEN];
    history_path(shell_home, path, sizeof(path));

    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        return;   /* no history file yet — that's fine */

    char   *line = NULL;
    size_t  len  = 0;
    ssize_t nread;

    while ((nread = getline(&line, &len, fp)) != -1) {
        /* Strip trailing newline */
        while (nread > 0 &&
               (line[nread - 1] == '\n' || line[nread - 1] == '\r'))
            line[--nread] = '\0';

        if (nread == 0)
            continue;

        if (hist_count >= MAX_LOG) {
            /* Shift out the oldest */
            free(history[0]);
            for (int i = 1; i < MAX_LOG; i++)
                history[i - 1] = history[i];
            hist_count = MAX_LOG - 1;
        }
        history[hist_count++] = strdup(line);
    }

    free(line);
    fclose(fp);
}

/*
 * log_save — Write all history entries to the persistent file.
 */
static void log_save(const char *shell_home)
{
    char path[MAX_PATH_LEN];
    history_path(shell_home, path, sizeof(path));

    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        perror("log_save");
        return;
    }

    for (int i = 0; i < hist_count; i++)
        fprintf(fp, "%s\n", history[i]);

    fclose(fp);
}

/* ------------------------------------------------------------------ */
/*  Public: add / query                                               */
/* ------------------------------------------------------------------ */

void log_add(const char *cmd, const char *shell_home)
{
    log_load(shell_home);

    /* Don't store if identical to the most recent entry */
    if (hist_count > 0 && strcmp(history[hist_count - 1], cmd) == 0)
        return;

    /* If full, drop the oldest entry */
    if (hist_count >= MAX_LOG) {
        free(history[0]);
        for (int i = 1; i < MAX_LOG; i++)
            history[i - 1] = history[i];
        hist_count = MAX_LOG - 1;
    }

    history[hist_count++] = strdup(cmd);
    log_save(shell_home);
}

/* ------------------------------------------------------------------ */
/*  contains_log_command                                              */
/* ------------------------------------------------------------------ */

/*
 * Return 1 if any atomic command in the shell_cmd line has "log"
 * as its first word (command name).
 *
 * Strategy: the first word of an atomic occurs either at the start
 * of the line or immediately after a ';', '&', or '|' separator
 * (skipping whitespace).
 */
int contains_log_command(const char *line)
{
    int   len = (int)strlen(line);
    int   i   = 0;

    while (i < len) {
        /* Skip whitespace */
        while (i < len && (line[i] == ' ' || line[i] == '\t' ||
                           line[i] == '\n' || line[i] == '\r'))
            i++;

        if (i >= len)
            break;

        /* Check if the word starting here is "log" */
        if (strncmp(line + i, "log", 3) == 0) {
            int after = i + 3;
            /* "log" must be a standalone word */
            if (after >= len || line[after] == ' '  ||
                line[after] == '\t' || line[after] == '\n' ||
                line[after] == '\r' || line[after] == ';'  ||
                line[after] == '&'  || line[after] == '|'  ||
                line[after] == '<'  || line[after] == '>') {
                return 1;
            }
        }

        /* Skip past this word to the next separator */
        while (i < len && line[i] != ';' && line[i] != '&' &&
               line[i] != '|')
            i++;

        /* Skip the separator itself */
        if (i < len)
            i++;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/*  builtin_log                                                       */
/* ------------------------------------------------------------------ */

void builtin_log(int argc, char **argv, ShellState *state)
{
    log_load(state->shell_home);

    if (argc == 1) {
        /* No arguments: print history oldest → newest */
        for (int i = 0; i < hist_count; i++)
            printf("%s\n", history[i]);
        return;
    }

    if (argc == 2 && strcmp(argv[1], "purge") == 0) {
        /* Purge: clear all history */
        for (int i = 0; i < hist_count; i++) {
            free(history[i]);
            history[i] = NULL;
        }
        hist_count = 0;
        log_save(state->shell_home);
        return;
    }

    if (argc == 3 && strcmp(argv[1], "execute") == 0) {
        /* Validate index */
        const char *idx_str = argv[2];
        for (int i = 0; idx_str[i] != '\0'; i++) {
            if (!isdigit((unsigned char)idx_str[i])) {
                printf("log: Invalid Syntax!\n");
                return;
            }
        }

        int index = atoi(idx_str);
        if (index < 1 || index > hist_count) {
            printf("log: Invalid Syntax!\n");
            return;
        }

        /* Index is 1-based, newest first.
         * So index 1 → history[hist_count - 1],
         *    index 2 → history[hist_count - 2], etc. */
        char *cmd = strdup(history[hist_count - index]);

        /* Execute the recalled command without storing it */
        state->skip_log = 1;

        /* Validate and execute */
        if (!validate_input(cmd)) {
            printf("Invalid Syntax!\n");
        } else {
            execute_line(cmd, state);
        }

        state->skip_log = 0;
        free(cmd);
        return;
    }

    /* Anything else is invalid */
    printf("log: Invalid Syntax!\n");
}
