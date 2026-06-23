/*
 * main.c — Shell entry point and REPL loop.
 *
 * 1. Records the current working directory as the shell's "home".
 * 2. Enters a loop:
 *    a) Display the prompt                      (prompt.c  — A.1)
 *    b) Read a line of user input               (input.c   — A.2)
 *    c) Validate the input against the grammar  (parser.c  — A.3)
 *    d) Store in log if appropriate              (log.c     — B.3)
 *    e) Execute the validated command            (execute.c — B/C)
 * 3. Exits on EOF (Ctrl-D).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "types.h"
#include "prompt.h"
#include "input.h"
#include "parser.h"
#include "execute.h"
#include "log.h"

int main(void)
{
    /* Initialise shell state */
    ShellState state;
    state.shell_home = getcwd(NULL, 0);
    state.prev_cwd   = NULL;
    state.skip_log   = 0;

    if (state.shell_home == NULL) {
        perror("getcwd");
        return 1;
    }

    /* ---- REPL loop ---- */
    for (;;) {
        display_prompt(state.shell_home);

        char *line = read_input();
        if (line == NULL) {
            /* EOF (Ctrl-D) — exit gracefully */
            printf("\n");
            break;
        }

        /* Skip empty / whitespace-only lines */
        {
            int all_space = 1;
            for (int i = 0; line[i] != '\0'; i++) {
                if (line[i] != ' '  && line[i] != '\t' &&
                    line[i] != '\n' && line[i] != '\r') {
                    all_space = 0;
                    break;
                }
            }
            if (all_space) {
                free(line);
                continue;
            }
        }

        /* Validate the input against the shell grammar (A.3) */
        if (!validate_input(line)) {
            printf("Invalid Syntax!\n");
            free(line);
            continue;
        }

        /* Store in log if appropriate (B.3):
         * - Not during a "log execute" recall (skip_log flag)
         * - Not if any atomic command in the line is "log" */
        if (!state.skip_log && !contains_log_command(line)) {
            log_add(line, state.shell_home);
        }

        /* Execute the validated command */
        execute_line(line, &state);

        free(line);
    }

    free(state.shell_home);
    free(state.prev_cwd);
    return 0;
}
