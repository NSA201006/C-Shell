/*
 * prompt.c — Shell prompt display (Part A.1).
 *
 * Implements display_prompt() which prints:
 *     <Username@SystemName:path>
 *
 * - Username comes from getenv("USER") (falls back to "user").
 * - SystemName comes from gethostname().
 * - path is the current working directory with the shell's home
 *   directory prefix replaced by '~' when applicable.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "prompt.h"
#include "types.h"

void display_prompt(const char *shell_home)
{
    /* ---- username ---- */
    const char *user = getenv("USER");
    if (user == NULL) {
        user = "user";
    }

    /* ---- hostname ---- */
    char hostname[MAX_HOSTNAME];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strncpy(hostname, "localhost", sizeof(hostname) - 1);
        hostname[sizeof(hostname) - 1] = '\0';
    }

    /* ---- current working directory ---- */
    char cwd[MAX_PATH_LEN];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        strncpy(cwd, "???", sizeof(cwd) - 1);
        cwd[sizeof(cwd) - 1] = '\0';
    }

    /* ---- home substitution ---- */
    const char *display_path = cwd;
    char tilde_path[MAX_PATH_LEN];

    size_t home_len = strlen(shell_home);
    if (strncmp(cwd, shell_home, home_len) == 0) {
        /*
         * cwd starts with shell_home.  Two valid cases:
         *   1) cwd == shell_home        → display "~"
         *   2) cwd == shell_home + "/" … → display "~/…"
         */
        if (cwd[home_len] == '\0') {
            display_path = "~";
        } else if (cwd[home_len] == '/') {
            snprintf(tilde_path, sizeof(tilde_path), "~%s",
                     cwd + home_len);
            display_path = tilde_path;
        }
        /* else: cwd merely shares a prefix (e.g. /home/abcdef vs
         *       /home/abc) — no substitution. */
    }

    /* ---- print prompt ---- */
    printf("<%s@%s:%s> ", user, hostname, display_path);
    fflush(stdout);
}
