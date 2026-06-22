/*
 * prompt.h — Shell prompt display (Part A.1).
 *
 * Provides a function to print the shell prompt in the format:
 *     <Username@SystemName:current_path>
 * with home-directory substitution (~ prefix).
 */

#ifndef PROMPT_H
#define PROMPT_H

/*
 * display_prompt — Print the shell prompt to stdout.
 *
 * @shell_home: absolute path of the directory in which the shell
 *              was originally started (the "home" directory).
 *
 * The current working directory is obtained internally via getcwd().
 * If cwd begins with shell_home, that prefix is replaced by '~'.
 */
void display_prompt(const char *shell_home);

#endif /* PROMPT_H */
