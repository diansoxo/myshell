#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ast.h"

ast_node_t *ast_create_node(node_type_t type) {
    ast_node_t *node = (ast_node_t*)malloc(sizeof(ast_node_t));
    if (!node) return NULL;
    
    node->type = type;
    node->left = NULL;
    node->right = NULL;
    node->argv = NULL;
    node->argc = 0;
    node->in_file = NULL;
    node->out_file = NULL;
    node->err_file = NULL;
    node->append = 0;
    node->redirect_err = 0;
    
    return node;
}

ast_node_t *ast_create_command_node(char **argv, int argc) {
    ast_node_t *node = ast_create_node(NODE_COMMAND);
    if (!node) return NULL;
    
    node->argv = argv;
    node->argc = argc;
    return node;
}

void ast_add_redirect(ast_node_t *node, char *file, int type, int is_append) {
    if (!node || node->type != NODE_COMMAND) return;
    
    switch (type) {
        case TOKEN_REDIR_IN:
            node->in_file = file;
            break;
        case TOKEN_REDIR_OUT:
            node->out_file = file;
            node->append = is_append;
            break;
        case TOKEN_REDIR_ERR:
            node->err_file = file;
            node->append = is_append;
            break;
    }
}

void ast_destroy(ast_node_t *node) {
    if (!node) return;
    
    ast_destroy(node->left);
    ast_destroy(node->right);
    
    if (node->argv) {
        for (int i = 0; i < node->argc; i++) {
            free(node->argv[i]);
        }
        free(node->argv);
    }
    
    free(node->in_file);
    free(node->out_file);
    free(node->err_file);
    
    free(node);
}

void ast_print(ast_node_t *node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth; i++) printf("  ");
    
    switch (node->type) {
        case NODE_COMMAND:
            printf("COMMAND: ");
            for (int i = 0; i < node->argc; i++) {
                printf("%s ", node->argv[i]);
            }
            if (node->in_file) printf("< %s ", node->in_file);
            if (node->out_file) printf(">%s %s ", node->append ? ">" : "", node->out_file);
            if (node->err_file) printf("2>%s %s ", node->append ? ">" : "", node->err_file);
            break;
        case NODE_PIPE:
            printf("PIPE");
            break;
        case NODE_REDIRECT:
            printf("REDIRECT");
            break;
        case NODE_AND:
            printf("AND");
            break;
        case NODE_OR:
            printf("OR");
            break;
        case NODE_SEMICOLON:
            printf("SEMICOLON");
            break;
        case NODE_BACKGROUND:
            printf("BACKGROUND");
            break;
        case NODE_SUBSHELL:
            printf("SUBSHELL");
            break;
    }
    printf("\n");
    
    ast_print(node->left, depth + 1);
    ast_print(node->right, depth + 1);
}
