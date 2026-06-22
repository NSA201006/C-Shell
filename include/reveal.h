/*
 * reveal.h — List directory contents (Part B.2).
 *
 * Syntax: reveal (-(a | l)*)* (~ | . | .. | - | name)?
 */

#ifndef REVEAL_H
#define REVEAL_H

#include "types.h"

/*
 * builtin_reveal — Execute the reveal command.
 *
 * @argc:  number of arguments (including "reveal" itself).
 * @argv:  argument array (argv[0] == "reveal").
 * @state: shell state (shell_home, prev_cwd).
 */
void builtin_reveal(int argc, char **argv, ShellState *state);

#endif /* REVEAL_H */
