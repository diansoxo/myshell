#ifndef AST_H
#define AST_H

typedef enum {
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_REDIR_APPEND,
    TOKEN_REDIR_ERR,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_SEMICOLON,
    TOKEN_BACKGROUND,
    TOKEN_EOF
} token_type_t;

typedef struct token {
    token_type_t type;
    char *value;
    struct token *next;
} token_t;

typedef enum {
    NODE_SIMPLE_COMMAND,
    NODE_PIPE,
    NODE_REDIRECT,
    NODE_AND,
    NODE_OR,
    NODE_SEMICOLON,
    NODE_BACKGROUND
} node_type_t;

typedef struct ast_node {
    node_type_t type;
    
    union {
        struct {
            char **args;
            int argc;
        } command;
        
        struct {
            struct ast_node *left;
            struct ast_node *right;
        } binary;
        

        struct {
            struct ast_node *child;
            char *filename;
            int flags;
            int fd;
        } redirect;
    } data;
} ast_node_t;

ast_node_t *create_command_node(char **args, int argc);
ast_node_t *create_binary_node(node_type_t type, ast_node_t *left, ast_node_t *right);
ast_node_t *create_redirect_node(ast_node_t *child, char *filename, int flags, int fd);
void free_ast(ast_node_t *node);

#endif
