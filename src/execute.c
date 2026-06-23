/*
 * execute.c — Command dispatch and execution.
 *
 * Splits a validated input line into individual commands
 * (delimited by ';' and '&'), tokenises each into an argv array,
 * and dispatches recognised builtins or runs external commands
 * via fork + execvp.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "execute.h"
#include "hop.h"
#include "log.h"
#include "reveal.h"
#include "types.h"

/*
 * is_ws — Return non-zero if c is whitespace.
 */
static int is_ws(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/*
 * is_meta — Return non-zero if c is a shell metacharacter.
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
 * Metacharacters (|, <, >, >>) are emitted as separate tokens.
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
 * build_exec_argv — From a full argv (which may contain <, >, >>),
 *                   extract only the command and its plain arguments,
 *                   skipping redirection tokens and their filenames.
 *
 * Returns a NULL-terminated array suitable for execvp().
 * Entries are NOT duplicated — they point into the original argv.
 */
static int build_exec_argv(int argc, char **argv, char **exec_argv)
{
    int n = 0;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "<") == 0 ||
            strcmp(argv[i], ">") == 0 ||
            strcmp(argv[i], ">>") == 0) {
            i++;   /* skip the filename that follows */
            continue;
        }
        exec_argv[n++] = argv[i];
    }
    exec_argv[n] = NULL;
    return n;
}

/*
 * setup_redirections — Handle <, >, >> in the argv before exec.
 *                      Must be called in the child process.
 *
 * Uses open() + dup2() + close() as required by the spec.
 * When multiple input (or output) redirections are present,
 * only the last one takes effect (left-to-right processing
 * with each dup2 overwriting the previous).
 *
 * Returns 0 on success, -1 on failure.
 */
static int setup_redirections(int argc, char **argv)
{
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "<") == 0 && i + 1 < argc) {
            /* Input redirection: open with O_RDONLY */
            int fd = open(argv[i + 1], O_RDONLY);
            if (fd < 0) {
                printf("No such file or directory\n");
                fflush(stdout);
                return -1;
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            i++;
        } else if (strcmp(argv[i], ">>") == 0 && i + 1 < argc) {
            /* Output append: O_WRONLY | O_CREAT | O_APPEND */
            int fd = open(argv[i + 1],
                          O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) {
                printf("Unable to create file for writing\n");
                fflush(stdout);
                return -1;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            i++;
        } else if (strcmp(argv[i], ">") == 0 && i + 1 < argc) {
            /* Output truncate: O_WRONLY | O_CREAT | O_TRUNC */
            int fd = open(argv[i + 1],
                          O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                printf("Unable to create file for writing\n");
                fflush(stdout);
                return -1;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            i++;
        }
    }
    return 0;
}

/*
 * run_external — Fork and exec an external command.
 */
static void run_external(int argc, char **argv)
{
    /* Build a clean argv for execvp (without redirection tokens) */
    char *exec_argv[MAX_ARGS];
    int exec_argc = build_exec_argv(argc, argv, exec_argv);

    if (exec_argc == 0)
        return;

    pid_t pid = fork();
    if (pid == 0) {
        /* Child process */
        /* Set up redirections first */
        if (setup_redirections(argc, argv) != 0)
            _exit(1);

        execvp(exec_argv[0], exec_argv);
        /* If we reach here, exec failed */
        printf("Command not found!\n");
        fflush(stdout);
        _exit(1);
    } else if (pid > 0) {
        /* Parent — wait for child */
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork");
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
    } else if (strcmp(argv[0], "log") == 0) {
        builtin_log(argc, argv, state);
    } else {
        run_external(argc, argv);
    }
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
        if (i == len || copy[i] == '&' || copy[i] == ';') {
            /* Extract the substring [start, i) */
            char saved = copy[i];
            copy[i] = '\0';

            char *segment = copy + start;

            /* Handle pipe groups: take only the first command
             * in a pipeline for builtin dispatch. */
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
