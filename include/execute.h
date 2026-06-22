/*
 * execute.h — Command dispatch and execution.
 *
 * Splits a validated input line into individual commands
 * (separated by ; and &) and dispatches builtins.
 */

#ifndef EXECUTE_H
#define EXECUTE_H

#include "types.h"

/*
 * execute_line — Execute a validated input line.
 *
 * Splits the line by ';' and '&' separators, then dispatches
 * each individual command to the appropriate builtin handler.
 *
 * @line:  the raw input string (already validated by parser).
 * @state: shell state.
 */
void execute_line(const char *line, ShellState *state);

#endif /* EXECUTE_H */
