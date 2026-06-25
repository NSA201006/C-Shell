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

#include "bg.h"
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
 * run_in_child — Execute a command in the current (child) process.
 *
 * Sets up file redirections, then either runs a builtin (and _exit)
 * or calls execvp.  This function never returns.
 */
static void run_in_child(int argc, char **argv, ShellState *state)
{
    /* Set up file redirections (<, >, >>) */
    if (setup_redirections(argc, argv) != 0)
        _exit(1);

    /* Build clean argv (strip redirection tokens) */
    char *exec_argv[MAX_ARGS];
    int exec_argc = build_exec_argv(argc, argv, exec_argv);

    if (exec_argc == 0)
        _exit(0);

    /* Builtins — run in-process then exit */
    if (strcmp(exec_argv[0], "hop") == 0) {
        builtin_hop(exec_argc, exec_argv, state);
        _exit(0);
    }
    if (strcmp(exec_argv[0], "reveal") == 0) {
        builtin_reveal(exec_argc, exec_argv, state);
        _exit(0);
    }
    if (strcmp(exec_argv[0], "log") == 0) {
        builtin_log(exec_argc, exec_argv, state);
        _exit(0);
    }

    /* External command */
    execvp(exec_argv[0], exec_argv);
    printf("Command not found!\n");
    fflush(stdout);
    _exit(1);
}

/*
 * run_external — Fork and exec a single external command (no pipes).
 */
static void run_external(int argc, char **argv, ShellState *state)
{
    char *exec_argv[MAX_ARGS];
    int exec_argc = build_exec_argv(argc, argv, exec_argv);

    if (exec_argc == 0)
        return;

    pid_t pid = fork();
    if (pid == 0) {
        run_in_child(argc, argv, state);
        /* never reached */
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork");
    }
}

/*
 * dispatch — Run a single simple command (no pipes).
 *            Builtins run in the current process; externals are forked.
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
        run_external(argc, argv, state);
    }
}

/*
 * run_pipeline — Execute a pipeline of commands connected by pipes.
 *
 * Creates (n_cmds - 1) pipes, forks a child for each command,
 * wires up stdin/stdout via dup2, and waits for all children.
 *
 * @stages:  array of command strings (one per pipeline stage).
 * @n_cmds:  number of stages.
 * @state:   shell state.
 */
static void run_pipeline(char **stages, int n_cmds, ShellState *state)
{
    /*
     * Flat pipe fd array: pipefds[2*i] = read end of pipe i,
     *                     pipefds[2*i+1] = write end of pipe i.
     */
    int n_pipes = n_cmds - 1;
    int pipefds[2 * n_pipes];

    for (int i = 0; i < n_pipes; i++) {
        if (pipe(pipefds + 2 * i) < 0) {
            perror("pipe");
            return;
        }
    }

    pid_t pids[MAX_ARGS];

    for (int i = 0; i < n_cmds; i++) {
        char *argv[MAX_ARGS];
        int argc = split_argv(stages[i], argv);

        pids[i] = fork();
        if (pids[i] == 0) {
            /* ---- Child process ---- */

            /* Wire up pipe endpoints */
            if (i > 0) {
                /* Read from previous pipe */
                dup2(pipefds[2 * (i - 1)], STDIN_FILENO);
            }
            if (i < n_pipes) {
                /* Write to current pipe */
                dup2(pipefds[2 * i + 1], STDOUT_FILENO);
            }

            /* Close ALL pipe fds in child */
            for (int j = 0; j < 2 * n_pipes; j++)
                close(pipefds[j]);

            /* Set up file redirections and exec */
            run_in_child(argc, argv, state);
            /* never reached */
        } else if (pids[i] < 0) {
            perror("fork");
        }

        free_argv(argv, argc);
    }

    /* ---- Parent: close all pipe fds ---- */
    for (int j = 0; j < 2 * n_pipes; j++)
        close(pipefds[j]);

    /* ---- Parent: wait for ALL children ---- */
    for (int i = 0; i < n_cmds; i++) {
        if (pids[i] > 0)
            waitpid(pids[i], NULL, 0);
    }
}

/*
 * extract_cmd_name — Get the first word of a segment for job reporting.
 */
static void extract_cmd_name(const char *seg, char *buf, size_t size)
{
    while (*seg && (*seg == ' ' || *seg == '\t'))
        seg++;
    size_t i = 0;
    while (seg[i] && seg[i] != ' ' && seg[i] != '\t' &&
           seg[i] != '|' && seg[i] != '<' && seg[i] != '>' &&
           i < size - 1) {
        buf[i] = seg[i];
        i++;
    }
    buf[i] = '\0';
}

/*
 * run_foreground — Execute a segment (single or pipeline) in foreground.
 */
static void run_foreground(char *segment, ShellState *state)
{
    int n_pipes = 0;
    for (char *p = segment; *p; p++)
        if (*p == '|') n_pipes++;

    if (n_pipes == 0) {
        char *argv[MAX_ARGS];
        int argc = split_argv(segment, argv);
        dispatch(argc, argv, state);
        free_argv(argv, argc);
    } else {
        char *stages[MAX_ARGS];
        int n_stages = 0;
        stages[n_stages++] = segment;
        for (char *p = segment; *p; p++) {
            if (*p == '|') {
                *p = '\0';
                stages[n_stages++] = p + 1;
            }
        }
        run_pipeline(stages, n_stages, state);
    }
}

/*
 * run_background — Fork and run a segment in the background.
 *
 * The child redirects stdin from /dev/null, then executes the
 * segment (single command or pipeline).  The parent records the
 * job and returns immediately.
 */
static void run_background(char *segment, ShellState *state)
{
    char cmd_name[256];
    extract_cmd_name(segment, cmd_name, sizeof(cmd_name));

    if (cmd_name[0] == '\0')
        return;

    pid_t pid = fork();
    if (pid == 0) {
        /* Child: no terminal input for background processes */
        int devnull = open("/dev/null", O_RDONLY);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            close(devnull);
        }

        /* Count pipes to decide single vs pipeline */
        int n_pipes = 0;
        for (char *p = segment; *p; p++)
            if (*p == '|') n_pipes++;

        if (n_pipes == 0) {
            /* Single command */
            char *argv[MAX_ARGS];
            int argc = split_argv(segment, argv);
            run_in_child(argc, argv, state);
            /* never returns */
        } else {
            /* Pipeline — run_pipeline forks its own children */
            char *stages[MAX_ARGS];
            int n_stages = 0;
            stages[n_stages++] = segment;
            for (char *p = segment; *p; p++) {
                if (*p == '|') {
                    *p = '\0';
                    stages[n_stages++] = p + 1;
                }
            }
            run_pipeline(stages, n_stages, state);
            _exit(0);
        }
    } else if (pid > 0) {
        bg_add_job(pid, cmd_name);
    } else {
        perror("fork");
    }
}

void execute_line(const char *line, ShellState *state)
{
    char *copy = strdup(line);
    if (copy == NULL)
        return;

    int len = (int)strlen(copy);
    int start = 0;

    for (int i = 0; i <= len; i++) {
        if (i == len || copy[i] == '&' || copy[i] == ';') {
            int background = (i < len && copy[i] == '&');

            char saved = copy[i];
            copy[i] = '\0';

            char *segment = copy + start;

            /* Check segment has content */
            int has_content = 0;
            for (char *p = segment; *p; p++) {
                if (*p != ' ' && *p != '\t' &&
                    *p != '\n' && *p != '\r') {
                    has_content = 1;
                    break;
                }
            }

            if (has_content) {
                if (background) {
                    run_background(segment, state);
                } else {
                    run_foreground(segment, state);
                }
            }

            copy[i] = saved;
            start = i + 1;
        }
    }

    free(copy);
}
