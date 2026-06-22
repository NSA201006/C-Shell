/*
 * input.h — User input reading (Part A.2).
 *
 * Provides a function to read one line of user input from stdin.
 */

#ifndef INPUT_H
#define INPUT_H

/*
 * read_input — Read a single line of user input.
 *
 * Uses getline() to handle arbitrary-length input.
 * Strips the trailing newline (if present).
 *
 * Returns:
 *   A heap-allocated string containing the input line (caller
 *   must free()), or NULL on EOF / read error.
 */
char *read_input(void);

#endif /* INPUT_H */
