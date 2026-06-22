/*
 * parser.h — Command-line tokeniser and parser (Part A.3).
 *
 * Validates user input against the shell's Context Free Grammar:
 *
 *   shell_cmd  → cmd_group (('&' | ';') cmd_group)* '&'?
 *   cmd_group  → atomic ('|' atomic)*
 *   atomic     → name (name | input | output)*
 *   input      → '<' name
 *   output     → ('>' | '>>') name
 *   name       → r"[^|&><;\s]+"
 */

#ifndef PARSER_H
#define PARSER_H

/*
 * validate_input — Check whether the given input line is valid
 *                  according to the shell grammar.
 *
 * @line: the raw input string (must not be NULL).
 *
 * Returns:
 *   1 if the input is syntactically valid.
 *   0 if the input is syntactically invalid.
 *
 * This function does NOT print any error messages itself;
 * the caller is responsible for printing "Invalid Syntax!"
 * when appropriate.
 */
int validate_input(const char *line);

#endif /* PARSER_H */
