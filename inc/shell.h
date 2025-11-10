#ifndef SHELL_H
#define SHELL_H

#include "lexer.h"
#include "parser.h"
#include "ast.h"

typedef struct {
    int running;
    char *prompt;
} shell_t;

shell_t *shell_create();
void shell_destroy(shell_t *shell);
void shell_run(shell_t *shell);

#endif