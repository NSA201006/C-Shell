/*
 * parser.c — Command-line tokeniser and recursive-descent parser (Part A.3).
 *
 * Phase 1: Tokenise the raw input string into an array of Tokens.
 * Phase 2: Walk the token array with a recursive-descent parser that
 *          validates the input against the shell grammar:
 *
 *   shell_cmd  → cmd_group (('&' | ';') cmd_group)* '&'?
 *   cmd_group  → atomic ('|' atomic)*
 *   atomic     → name (name | input | output)*
 *   input      → '<' name
 *   output     → ('>' | '>>') name
 *   name       → r"[^|&><;\s]+"
 *
 * All whitespace between tokens is ignored by the tokeniser.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "types.h"

/* ------------------------------------------------------------------ */
/*  Tokeniser                                                         */
/* ------------------------------------------------------------------ */

/*
 * is_special — Return non-zero if c is a shell metacharacter.
 */
static int is_special(char c)
{
    return c == '|' || c == '&' || c == '>' ||
           c == '<' || c == ';';
}

/*
 * is_whitespace — Return non-zero if c is whitespace.
 */
static int is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/*
 * tokenize — Split a raw input string into an array of tokens.
 *
 * @input:   the input string (not modified).
 * @tokens:  output array (caller-provided, at least MAX_TOKENS elements).
 * @count:   output — number of tokens stored (excluding the sentinel).
 *
 * Returns:
 *   1 on success, 0 on tokenisation failure (e.g. too many tokens).
 *
 * The last element written is always a TOKEN_EOF sentinel.
 */
static int tokenize(const char *input, Token *tokens, int *count)
{
    int   n   = 0;
    int   i   = 0;
    int   len = (int)strlen(input);

    while (i < len) {
        /* skip whitespace */
        if (is_whitespace(input[i])) {
            i++;
            continue;
        }

        if (n >= MAX_TOKENS - 1) {
            return 0;   /* too many tokens */
        }

        /* '>>' (must check before single '>') */
        if (input[i] == '>' && i + 1 < len && input[i + 1] == '>') {
            tokens[n].type  = TOKEN_APPEND;
            tokens[n].value = NULL;
            n++;
            i += 2;
            continue;
        }

        /* single-character operators */
        if (input[i] == '|') {
            tokens[n].type  = TOKEN_PIPE;
            tokens[n].value = NULL;
            n++; i++;
            continue;
        }
        if (input[i] == '&') {
            tokens[n].type  = TOKEN_AMP;
            tokens[n].value = NULL;
            n++; i++;
            continue;
        }
        if (input[i] == ';') {
            tokens[n].type  = TOKEN_SEMI;
            tokens[n].value = NULL;
            n++; i++;
            continue;
        }
        if (input[i] == '<') {
            tokens[n].type  = TOKEN_LT;
            tokens[n].value = NULL;
            n++; i++;
            continue;
        }
        if (input[i] == '>') {
            tokens[n].type  = TOKEN_GT;
            tokens[n].value = NULL;
            n++; i++;
            continue;
        }

        /* word: a run of non-special, non-whitespace characters */
        {
            int start = i;
            while (i < len && !is_whitespace(input[i]) &&
                   !is_special(input[i])) {
                i++;
            }
            int word_len = i - start;
            char *word = malloc((size_t)word_len + 1);
            if (word == NULL) {
                return 0;
            }
            memcpy(word, input + start, (size_t)word_len);
            word[word_len] = '\0';

            tokens[n].type  = TOKEN_WORD;
            tokens[n].value = word;
            n++;
        }
    }

    /* sentinel */
    tokens[n].type  = TOKEN_EOF;
    tokens[n].value = NULL;
    *count = n;
    return 1;
}

/*
 * free_tokens — Release all heap memory held by word tokens.
 */
static void free_tokens(Token *tokens, int count)
{
    for (int i = 0; i < count; i++) {
        if (tokens[i].type == TOKEN_WORD) {
            free(tokens[i].value);
            tokens[i].value = NULL;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Recursive-descent parser                                          */
/* ------------------------------------------------------------------ */

/*
 * The parser state is simply the current position (pos) in the
 * token array.  Each parse_* function advances pos and returns
 * 1 (success) or 0 (failure).
 */

static int parse_cmd_group(Token *tokens, int *pos);

/*
 * current — Return the type of the token at the current position.
 */
static TokenType current(Token *tokens, int pos)
{
    return tokens[pos].type;
}

/*
 * parse_atomic — Parse an 'atomic' production.
 *
 *   atomic → name (name | input | output)*
 *   input  → '<' name
 *   output → ('>' | '>>') name
 *
 * The first token MUST be a WORD (the command name).
 */
static int parse_atomic(Token *tokens, int *pos)
{
    /* The leading name is mandatory */
    if (current(tokens, *pos) != TOKEN_WORD) {
        return 0;
    }
    (*pos)++;

    /* Zero or more arguments / redirections */
    for (;;) {
        TokenType t = current(tokens, *pos);

        if (t == TOKEN_WORD) {
            /* plain argument */
            (*pos)++;
        } else if (t == TOKEN_LT) {
            /* input redirection: '<' name */
            (*pos)++;
            if (current(tokens, *pos) != TOKEN_WORD) {
                return 0;
            }
            (*pos)++;
        } else if (t == TOKEN_GT || t == TOKEN_APPEND) {
            /* output redirection: '>' name  |  '>>' name */
            (*pos)++;
            if (current(tokens, *pos) != TOKEN_WORD) {
                return 0;
            }
            (*pos)++;
        } else {
            /* not part of this atomic */
            break;
        }
    }

    return 1;
}

/*
 * parse_cmd_group — Parse a 'cmd_group' production.
 *
 *   cmd_group → atomic ('|' atomic)*
 */
static int parse_cmd_group(Token *tokens, int *pos)
{
    if (!parse_atomic(tokens, pos)) {
        return 0;
    }

    while (current(tokens, *pos) == TOKEN_PIPE) {
        (*pos)++;   /* consume '|' */
        if (!parse_atomic(tokens, pos)) {
            return 0;
        }
    }

    return 1;
}

/*
 * parse_shell_cmd — Parse a 'shell_cmd' production (the top level).
 *
 *   shell_cmd → cmd_group (('&' | ';') cmd_group)* '&'?
 */
static int parse_shell_cmd(Token *tokens, int *pos)
{
    if (!parse_cmd_group(tokens, pos)) {
        return 0;
    }

    while (current(tokens, *pos) == TOKEN_AMP ||
           current(tokens, *pos) == TOKEN_SEMI) {
        (*pos)++;   /* consume '&' or ';' */

        /*
         * A trailing '&' (with nothing after it) is valid.
         * We need to check: if we consumed '&' and the next token
         * is EOF, that is the optional trailing '&', so we are done.
         *
         * But if we consumed ';' and the next token is EOF, that is
         * invalid (';' must be followed by a cmd_group).
         *
         * Actually let's re-check: the consumed token could be '&'.
         * If it was '&' and next is EOF → valid (trailing &).
         * If it was ';' and next is EOF → invalid.
         * If next is not EOF → must parse another cmd_group.
         */
        TokenType consumed = tokens[*pos - 1].type;

        if (current(tokens, *pos) == TOKEN_EOF) {
            if (consumed == TOKEN_AMP) {
                return 1;   /* valid trailing '&' */
            }
            return 0;       /* trailing ';' with no cmd_group */
        }

        /* Another '&' or ';' right after → need to check grammar.
         * Actually the grammar says (('&'|';') cmd_group)*, so
         * we always expect a cmd_group next.  But if the next token
         * is '&' and after that is EOF, that could be the trailing '&'.
         * However per the grammar, the trailing '&' only comes after
         * the last cmd_group.  So here we must have a cmd_group. */

        if (!parse_cmd_group(tokens, pos)) {
            return 0;
        }
    }

    return 1;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

int validate_input(const char *line)
{
    Token tokens[MAX_TOKENS];
    int   count = 0;

    /* Phase 1: tokenise */
    if (!tokenize(line, tokens, &count)) {
        free_tokens(tokens, count);
        return 0;
    }

    /* Empty input is "valid" (the shell just re-prompts) */
    if (count == 0) {
        return 1;
    }

    /* Phase 2: parse */
    int pos   = 0;
    int valid = parse_shell_cmd(tokens, &pos);

    /* All tokens must have been consumed */
    if (valid && current(tokens, pos) != TOKEN_EOF) {
        valid = 0;
    }

    free_tokens(tokens, count);
    return valid;
}
