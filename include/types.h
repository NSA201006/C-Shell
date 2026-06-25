/*
 * types.h — Shared type definitions and constants for the shell.
 *
 * This header provides common buffer sizes and token type
 * definitions used across all shell modules.
 */

#ifndef TYPES_H
#define TYPES_H

/* Maximum length of a single input line */
#define MAX_INPUT    4096

/* Maximum length for hostname */
#define MAX_HOSTNAME 256

/* Maximum length for paths */
#define MAX_PATH_LEN 4096

/* Maximum number of tokens in a single input line */
#define MAX_TOKENS   512

#define MAX_BG_JOBS  256

/*
 * Token types produced by the tokeniser (parser.c).
 *
 * TOKEN_WORD   — a plain word (command name, argument, filename)
 * TOKEN_PIPE   — the '|' operator
 * TOKEN_AMP    — the '&' operator
 * TOKEN_SEMI   — the ';' operator
 * TOKEN_LT     — the '<' input-redirection operator
 * TOKEN_GT     — the '>' output-redirection operator
 * TOKEN_APPEND — the '>>' append-redirection operator
 * TOKEN_EOF    — sentinel marking the end of the token array
 */
typedef enum {
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_AMP,
    TOKEN_SEMI,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_APPEND,
    TOKEN_EOF
} TokenType;

/*
 * A single token: its type plus an optional string value.
 * Only TOKEN_WORD tokens carry a non-NULL value.
 */
typedef struct {
    TokenType type;
    char     *value;   /* heap-allocated for TOKEN_WORD; NULL otherwise */
} Token;

/* Maximum number of arguments in a single command */
#define MAX_ARGS     256

/* Maximum number of commands stored in log history */
#define MAX_LOG      15

/* Name of the persistent history file (stored in shell_home) */
#define LOG_FILE     ".shell_history"

/*
 * ShellState — Persistent state shared across all shell modules.
 *
 * shell_home: absolute path of the directory where the shell started.
 * prev_cwd:   previous working directory (NULL until the first hop).
 * skip_log:   when non-zero, do not store the current command in log
 *             (used by log execute to prevent re-storing recalled cmds).
 */
typedef struct {
    char *shell_home;
    char *prev_cwd;
    int   skip_log;
} ShellState;

#endif /* TYPES_H */
