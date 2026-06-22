/*
 * hop.c — Change working directory builtin (Part B.1).
 *
 * Syntax: hop ((~ | . | .. | - | name)*)?
 *
 * Processes each argument sequentially:
 *   ~  or no args → shell home
 *   .             → stay (no-op)
 *   ..            → parent directory
 *   -             → previous CWD (no-op if none yet)
 *   name          → chdir to the given path
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hop.h"
#include "types.h"

/*
 * hop_to — Attempt to change directory to the given path.
 *
 * On success, updates state->prev_cwd to the old CWD.
 * On failure, prints "No such directory!".
 */
static void hop_to(const char *path, ShellState *state)
{
    char *old_cwd = getcwd(NULL, 0);
    if (old_cwd == NULL) {
        perror("getcwd");
        return;
    }

    if (chdir(path) != 0) {
        printf("No such directory!\n");
        free(old_cwd);
        return;
    }

    /* Update prev_cwd */
    free(state->prev_cwd);
    state->prev_cwd = old_cwd;
}

void builtin_hop(int argc, char **argv, ShellState *state)
{
    /* No arguments → go to shell home */
    if (argc == 1) {
        hop_to(state->shell_home, state);
        return;
    }

    /* Process each argument sequentially */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "~") == 0) {
            hop_to(state->shell_home, state);
        } else if (strcmp(argv[i], ".") == 0) {
            /* Do nothing */
        } else if (strcmp(argv[i], "..") == 0) {
            hop_to("..", state);
        } else if (strcmp(argv[i], "-") == 0) {
            if (state->prev_cwd != NULL) {
                hop_to(state->prev_cwd, state);
            }
            /* else: no previous CWD → do nothing */
        } else {
            /* Treat as a relative or absolute path */
            /* Handle ~/... paths */
            if (argv[i][0] == '~' && argv[i][1] == '/') {
                char fullpath[MAX_PATH_LEN];
                snprintf(fullpath, sizeof(fullpath), "%s%s",
                         state->shell_home, argv[i] + 1);
                hop_to(fullpath, state);
            } else {
                hop_to(argv[i], state);
            }
        }
    }
}
