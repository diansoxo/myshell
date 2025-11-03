#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ast.h"

ast_node_t *ast_create_node(node_type_t type) {
    ast_node_t *node = (ast_node_t*)malloc(sizeof(ast_node_t));
    if (node == NULL) {
        return NULL;
    }

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
    if (node == NULL) {
        return NULL;
    }
    
    node->argv = argv;
    node->argc = argc;
    
    return node;
}

void ast_destroy(ast_node_t *node) {
    if (node == NULL) {
        return;
    }
    
    ast_destroy(node->left);
    ast_destroy(node->right);
    
    
    if (node->argv != NULL) {
        for (int i = 0; i < node->argc; i++) {
            if (node->argv[i] != NULL) {
                free(node->argv[i]);
            }
        }
        free(node->argv);
    }

    
    if (node->in_file != NULL) {
        free(node->in_file);
    }
    if (node->out_file != NULL) {
        free(node->out_file);
    }
    if (node->err_file != NULL) {
        free(node->err_file);
    }
    
    free(node);
}


static const char* node_type_to_string(node_type_t type) {
    switch (type) {
        case NODE_COMMAND: return "COMMAND";
        case NODE_PIPE: return "PIPE";
        case NODE_REDIRECT: return "REDIRECT";
        case NODE_AND: return "AND";
        case NODE_OR: return "OR";
        case NODE_SEMICOLON: return "SEMICOLON";
        case NODE_BACKGROUND: return "BACKGROUND";
        case NODE_SUBSHELL: return "SUBSHELL";
        default: return "UNKNOWN";
    }
}

void ast_print(ast_node_t *node, int depth) {
    if (node == NULL) {
        return;
    }
    
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    
    printf("%s", node_type_to_string(node->type));
    

    switch (node->type) {
        case NODE_COMMAND:
            if (node->argc > 0) {
                printf(" [");
                for (int i = 0; i < node->argc; i++) {
                    printf("%s", node->argv[i]);
                    if (i < node->argc - 1) {
                        printf(" ");
                    }
                }
                printf("]");
            }
            break;
            
        case NODE_PIPE:
            if (node->redirect_err) {
                printf(" (|& stderr redirect)");
            }
            break;
            
        case NODE_REDIRECT:
            break;
            
        default:
            break;
    }
    
    if (node->in_file != NULL) {
        printf(" < %s", node->in_file);
    }
    if (node->out_file != NULL) {
        printf(" %s %s", node->append ? ">>" : ">", node->out_file);
    }
    if (node->err_file != NULL) {
        printf(" %s %s", node->append ? "&>>" : "&>", node->err_file);
    }
    
    printf("\n");
    
    ast_print(node->left, depth + 1);
    ast_print(node->right, depth + 1);
}

char **ast_clone_argv(char **argv, int argc) {
    if (argv == NULL || argc <= 0) {
        return NULL;
    }
    
    char **new_argv = (char**)malloc(argc * sizeof(char*));
    if (new_argv == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < argc; i++) {
        new_argv[i] = strdup(argv[i]);
        if (new_argv[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(new_argv[j]);
            }
            free(new_argv);
            return NULL;
        }
    }
    
    return new_argv;
}

int ast_is_command(ast_node_t *node, const char *cmd_name) {
    if (node == NULL || node->type != NODE_COMMAND || node->argc == 0) {
        return 0;
    }
    
    return strcmp(node->argv[0], cmd_name) == 0;
}

const char *ast_get_command_name(ast_node_t *node) {
    if (node == NULL || node->type != NODE_COMMAND || node->argc == 0) {
        return NULL;
    }
    
    return node->argv[0];
}
