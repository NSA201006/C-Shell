/*
 * input.c — User input reading (Part A.2).
 *
 * Implements read_input() which reads one line from stdin using
 * the POSIX getline() function.  The trailing newline is stripped
 * before returning.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "input.h"

char *read_input(void)
{
    char   *line = NULL;
    size_t  len  = 0;
    ssize_t nread;

    nread = getline(&line, &len, stdin);

    if (nread == -1) {
        /* EOF or error */
        free(line);
        return NULL;
    }

    /* Strip trailing newline / carriage-return */
    while (nread > 0 &&
           (line[nread - 1] == '\n' || line[nread - 1] == '\r')) {
        line[--nread] = '\0';
    }

    return line;
}
