/*
 * hop.h — Change working directory (Part B.1).
 *
 * Syntax: hop ((~ | . | .. | - | name)*)?
 */

#ifndef HOP_H
#define HOP_H

#include "types.h"

/*
 * builtin_hop — Execute the hop command.
 *
 * @argc:  number of arguments (including "hop" itself).
 * @argv:  argument array (argv[0] == "hop").
 * @state: shell state (shell_home, prev_cwd).
 */
void builtin_hop(int argc, char **argv, ShellState *state);

#endif /* HOP_H */
