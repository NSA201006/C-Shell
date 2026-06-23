/*
 * log.h — Command history builtin (Part B.3).
 *
 * Syntax: log (purge | execute <index>)?
 */

#ifndef LOG_H
#define LOG_H

#include "types.h"

/*
 * builtin_log — Execute the log command.
 *
 * @argc:  number of arguments (including "log" itself).
 * @argv:  argument array (argv[0] == "log").
 * @state: shell state.
 */
void builtin_log(int argc, char **argv, ShellState *state);

/*
 * contains_log_command — Check if any atomic in the shell_cmd
 *                        has "log" as its command name.
 *
 * Returns 1 if "log" appears as a command name, 0 otherwise.
 */
int contains_log_command(const char *line);

/*
 * log_add — Add a command to the persistent history.
 *
 * Skips storage if the command is identical to the most recent entry.
 * Overwrites the oldest entry when the history is full (MAX_LOG).
 */
void log_add(const char *cmd, const char *shell_home);

#endif /* LOG_H */
