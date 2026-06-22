/*
 * main.c — Shell entry point and REPL loop.
 *
 * 1. Records the current working directory as the shell's "home".
 * 2. Enters a loop:
 *    a) Display the prompt                      (prompt.c  — A.1)
 *    b) Read a line of user input               (input.c   — A.2)
 *    c) Validate the input against the grammar  (parser.c  — A.3)
 *    d) Execute the validated command            (execute.c — B)
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

int main(void)
{
    /* Initialise shell state */
    ShellState state;
    state.shell_home = getcwd(NULL, 0);
    state.prev_cwd   = NULL;

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

        /* Execute the validated command (B) */
        execute_line(line, &state);

        free(line);
    }

    free(state.shell_home);
    free(state.prev_cwd);
    return 0;
}
