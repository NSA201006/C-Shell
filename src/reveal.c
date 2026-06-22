/*
 * reveal.c — List directory contents builtin (Part B.2).
 *
 * Syntax: reveal (-(a | l)*)* (~ | . | .. | - | name)?
 *
 * Flags:
 *   -a : show hidden files (starting with '.')
 *   -l : one entry per line
 *
 * Path resolution is identical to hop (~ . .. - name).
 * Default (no path) lists the current directory.
 * Files are sorted in ASCII lexicographic order using strcmp.
 */

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reveal.h"
#include "types.h"

/*
 * cmp_strings — Comparison function for qsort (ASCII order).
 */
static int cmp_strings(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

/*
 * is_flag — Return 1 if the argument is a valid flag group.
 *
 * A flag starts with '-' and every subsequent character is 'a' or 'l'.
 * A bare '-' (no characters after it) is NOT a flag — it's a path.
 */
static int is_flag(const char *arg)
{
    if (arg[0] != '-' || arg[1] == '\0') {
        return 0;
    }
    for (int i = 1; arg[i] != '\0'; i++) {
        if (arg[i] != 'a' && arg[i] != 'l') {
            return 0;
        }
    }
    return 1;
}

/*
 * list_directory — Read, sort, and print directory entries.
 */
static void list_directory(const char *path, int flag_a, int flag_l)
{
    DIR *dir = opendir(path);
    if (dir == NULL) {
        printf("No such directory!\n");
        return;
    }

    /* Collect entries into a dynamic array */
    char **entries = NULL;
    int count = 0;
    int capacity = 0;
    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL) {
        /* Skip hidden files unless -a is set */
        if (!flag_a && ent->d_name[0] == '.') {
            continue;
        }

        /* Grow array if needed */
        if (count >= capacity) {
            capacity = capacity == 0 ? 64 : capacity * 2;
            entries = realloc(entries, (size_t)capacity * sizeof(char *));
        }
        entries[count] = strdup(ent->d_name);
        count++;
    }
    closedir(dir);

    /* Sort in ASCII lexicographic order */
    if (count > 0) {
        qsort(entries, (size_t)count, sizeof(char *), cmp_strings);
    }

    /* Print entries */
    for (int i = 0; i < count; i++) {
        if (flag_l) {
            printf("%s\n", entries[i]);
        } else {
            if (i > 0) {
                printf(" ");
            }
            printf("%s", entries[i]);
        }
        free(entries[i]);
    }

    /* If not -l mode and we printed something, end with a newline */
    if (!flag_l && count > 0) {
        printf("\n");
    }

    free(entries);
}

void builtin_reveal(int argc, char **argv, ShellState *state)
{
    int flag_a = 0;
    int flag_l = 0;
    char *target = NULL;
    int path_count = 0;

    /* Parse flags and path argument */
    for (int i = 1; i < argc; i++) {
        if (is_flag(argv[i])) {
            /* Extract individual flags */
            for (int j = 1; argv[i][j] != '\0'; j++) {
                if (argv[i][j] == 'a') flag_a = 1;
                if (argv[i][j] == 'l') flag_l = 1;
            }
        } else {
            target = argv[i];
            path_count++;
        }
    }

    /* Too many path arguments */
    if (path_count > 1) {
        printf("reveal: Invalid Syntax!\n");
        return;
    }

    /* Resolve target directory path */
    const char *dir_path = ".";   /* default: current directory */
    char fullpath[MAX_PATH_LEN];

    if (target != NULL) {
        if (strcmp(target, "~") == 0) {
            dir_path = state->shell_home;
        } else if (strcmp(target, "-") == 0) {
            if (state->prev_cwd == NULL) {
                printf("No such directory!\n");
                return;
            }
            dir_path = state->prev_cwd;
        } else if (target[0] == '~' && target[1] == '/') {
            snprintf(fullpath, sizeof(fullpath), "%s%s",
                     state->shell_home, target + 1);
            dir_path = fullpath;
        } else {
            /* . , .. , or any other relative/absolute path */
            dir_path = target;
        }
    }

    list_directory(dir_path, flag_a, flag_l);
}
