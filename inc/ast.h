#ifndef AST_H
#define AST_H
#include "tokens.h"

typedef enum {
    NODE_COMMAND,//
    NODE_PIPE,//
    NODE_REDIRECT, //
    NODE_AND,
    NODE_OR,
    NODE_SEMICOLON,
    NODE_BACKGROUND,
    NODE_SUBSHELL
} node_type_t;


typedef struct {// изм разделили данные на отдельные структуры
    char **argv;
    int argc;
} command_data_t;// Только для команд

typedef struct {
    char *in_file;
    char *out_file;
    char *err_file;
    int append;
} redirect_data_t;// Только для перенаправлений

typedef struct {
    int redirect_err;// Только для пайпов
} pipe_data_t;


typedef struct ast_node_t {
    node_type_t type;
    struct ast_node_t *left;
    struct ast_node_t *right;
    
    union {
        command_data_t command;// Исп только когда type == NODE_COMMAND
        pipe_data_t pipe;//когда type == NODE_PIPE  
        redirect_data_t redirect;//когда type == NODE_REDIRECT
    } data;//для остальных типов не нужно дополнительных данных
} ast_node_t;

ast_node_t *ast_create_node(node_type_t type);
void ast_destroy(ast_node_t *node);
ast_node_t *ast_create_command_node(char **argv, int argc);
void ast_add_redirect(ast_node_t *node, char *file, int type, int is_append);
void ast_print(ast_node_t *node, int depth);

#endif
