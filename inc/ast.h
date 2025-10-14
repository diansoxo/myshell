#ifndef AST_H
#define AST_H
#include "tokens.h"

typedef enum {
    NODE_COMMAND,
    NODE_PIPE,
    NODE_REDIRECT,
    NODE_AND,
    NODE_OR,
    NODE_SEMICOLON,
    NODE_BACKGROUND,
    NODE_SUBSHELL
} node_type_t;

typedef struct ast_node_t {
    node_type_t type;
    struct ast_node_t *left;
    struct ast_node_t *right;
    
    // Для команд
    char **args;
    int argc;
    
    // Для перенаправлений
    char *in_file;
    char *out_file;
    char *err_file;
    int append;
    int redirect_err;
} ast_node_t;

ast_node_t *ast_create_node(node_type_t type);
void ast_destroy(ast_node_t *node);
ast_node_t *ast_create_command_node(char **args, int argc);

#endif