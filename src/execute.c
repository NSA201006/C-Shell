/*
 * execute.c — Command dispatch and execution.
 *
 * Splits a validated input line into individual commands
 * (delimited by ';' and '&'), tokenises each into an argv array,
 * and dispatches recognised builtins (hop, reveal).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "execute.h"
#include "hop.h"
#include "reveal.h"
#include "types.h"

/*
 * is_whitespace — Return non-zero if c is whitespace.
 */
static int is_ws(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/*
 * is_meta — Return non-zero if c is a shell metacharacter
 *           (separator or redirection).
 */
static int is_meta(char c)
{
    return c == '|' || c == '&' || c == '>' ||
           c == '<' || c == ';';
}

/*
 * split_argv — Split a simple command string into an argv array.
 *
 * Words are delimited by whitespace and metacharacters.
 * Metacharacters (|, <, >, >>) are emitted as separate tokens
 * so that builtins can ignore them if needed.
 *
 * @cmd:   the command substring (will NOT be modified).
 * @argv:  output array (caller-provided, MAX_ARGS elements).
 *
 * Returns the number of tokens (argc).
 * All argv entries are heap-allocated; caller must free them.
 */
static int split_argv(const char *cmd, char **argv)
{
    int argc = 0;
    int i = 0;
    int len = (int)strlen(cmd);

    while (i < len && argc < MAX_ARGS - 1) {
        /* Skip whitespace */
        while (i < len && is_ws(cmd[i]))
            i++;
        if (i >= len)
            break;

        /* '>>' special case */
        if (cmd[i] == '>' && i + 1 < len && cmd[i + 1] == '>') {
            argv[argc++] = strdup(">>");
            i += 2;
            continue;
        }

        /* Single-char metacharacter */
        if (is_meta(cmd[i])) {
            char buf[2] = { cmd[i], '\0' };
            argv[argc++] = strdup(buf);
            i++;
            continue;
        }

        /* Word: run of non-whitespace, non-meta chars */
        int start = i;
        while (i < len && !is_ws(cmd[i]) && !is_meta(cmd[i]))
            i++;
        int wlen = i - start;
        char *word = malloc((size_t)wlen + 1);
        memcpy(word, cmd + start, (size_t)wlen);
        word[wlen] = '\0';
        argv[argc++] = word;
    }

    argv[argc] = NULL;
    return argc;
}

/*
 * free_argv — Free all heap-allocated strings in argv.
 */
static void free_argv(char **argv, int argc)
{
    for (int i = 0; i < argc; i++) {
        free(argv[i]);
    }
}

/*
 * dispatch — Run a single simple command (argv[0] decides).
 */
static void dispatch(int argc, char **argv, ShellState *state)
{
    if (argc == 0)
        return;

    if (strcmp(argv[0], "hop") == 0) {
        builtin_hop(argc, argv, state);
    } else if (strcmp(argv[0], "reveal") == 0) {
        builtin_reveal(argc, argv, state);
    }
    /* Unknown commands are silently ignored for now (exec* banned). */
}

void execute_line(const char *line, ShellState *state)
{
    /*
     * Split the input by ';' and '&' to get individual command
     * groups.  We work on a mutable copy of the line.
     */
    char *copy = strdup(line);
    if (copy == NULL)
        return;

    int len = (int)strlen(copy);
    int start = 0;

    for (int i = 0; i <= len; i++) {
        /*
         * We split on '&', ';', or end-of-string.
         * Skip '>>' so we don't confuse '>' with a separator
         * (but '>' is never a separator — only '&' and ';' are).
         */
        if (i == len || copy[i] == '&' || copy[i] == ';') {
            /* Extract the substring [start, i) */
            char saved = copy[i];
            copy[i] = '\0';

            /* Handle pipe groups: take only the first command
             * in a pipeline for builtin dispatch.  Piped stages
             * cannot run without exec*, so we just run the first. */
            char *segment = copy + start;

            /* Find the first '|' in the segment */
            char *pipe_pos = strchr(segment, '|');
            if (pipe_pos != NULL) {
                *pipe_pos = '\0';
            }

            /* Tokenise and dispatch */
            char *argv[MAX_ARGS];
            int argc = split_argv(segment, argv);
            dispatch(argc, argv, state);
            free_argv(argv, argc);

            copy[i] = saved;
            start = i + 1;
        }
    }

    free(copy);
}
