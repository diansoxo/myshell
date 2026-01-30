#include <stdlib.h>//добавила switch для инициализации union
#include <string.h>
#include <stdio.h>
#include "ast.h"


ast_node_t *ast_create_node(node_type_t type) {//создание узла для разных типов операторов измм (созд узел нужного типа)
    ast_node_t *node = (ast_node_t*)malloc(sizeof(ast_node_t));
    if (node == NULL) {
        return NULL;
    }
    node->type = type;
    node->left = NULL;
    node->right = NULL;
    
    
    switch (type) {//Поля инициализируются только в соответствующих частях union
        case NODE_COMMAND:
            node->data.command.argv = NULL;
            node->data.command.argc = 0;
            break;
        case NODE_PIPE:
            node->data.pipe.redirect_err = 0;
            break;
        case NODE_REDIRECT:
            node->data.redirect.in_file = NULL;
            node->data.redirect.out_file = NULL;
            node->data.redirect.err_file = NULL;
            node->data.redirect.append = 0;
            break;
        default:// Для остальных типов ничего не инициализируем
            break;
    }
    return node;
}

//доступ через data.command
ast_node_t *ast_create_command_node(char **argv, int argc) {// Создание узла команды с аргументами изм
    ast_node_t *node = ast_create_node(NODE_COMMAND);
    if (node == NULL) {
        return NULL;
    }
    node->data.command.argv = argv;
    node->data.command.argc = argc;
    return node;
}
//добавила switch и доступ через union
void ast_destroy(ast_node_t *node) {// Рекурсивное уничтожение AST дерева
    if (node == NULL) {
        return;
    }
    switch (node->type) {
        case NODE_COMMAND:
            if (node->data.command.argv != NULL) {
                for (int i = 0; i < node->data.command.argc; i++) {
                    if (node->data.command.argv[i] != NULL) {
                        free(node->data.command.argv[i]);
                    }
                }
                free(node->data.command.argv);
            }
            break;
        
        case NODE_REDIRECT:
            if (node->data.redirect.in_file != NULL) {
                free(node->data.redirect.in_file);
            }
            if (node->data.redirect.out_file != NULL) {
                free(node->data.redirect.out_file);
            }
            if (node->data.redirect.err_file != NULL) {
                free(node->data.redirect.err_file);
            }
            break;
        
        default:
            break;
    }
    if (node->left != NULL) {
        ast_destroy(node->left);
    }
    if (node->right != NULL) {
        ast_destroy(node->right);
    }
    free(node);
}


// все обращения через union
void ast_print(ast_node_t *node, int depth) {// Рекурсивная печать AST для отладки изм
    if (node == NULL) {
        return;
    }
    
    // Печатаем отступ для визуализации глубины дерева
    for (int i = 0; i < depth; i++) {  // FIX: Now using depth parameter
        printf("  ");
    }
    
    printf("Node type: ");
    switch (node->type) {
        case NODE_COMMAND:
            printf("COMMAND");
            if (node->data.command.argc > 0) {
                printf(" [");
                for (int i = 0; i < node->data.command.argc; i++) {
                    printf("%s", node->data.command.argv[i]);
                    if (i < node->data.command.argc - 1) {
                        printf(" ");
                    }
                }
                printf("]");
            }
            break;
        
        case NODE_PIPE:
            printf("PIPE");
            if (node->data.pipe.redirect_err) {
                printf(" (|& stderr redirect)");
            }
            break;
        
        case NODE_REDIRECT:
            printf("REDIRECT");
            if (node->data.redirect.in_file != NULL) {
                printf(" < %s", node->data.redirect.in_file);
            }
            if (node->data.redirect.out_file != NULL) {
                printf(" %s %s", 
                    node->data.redirect.append ? ">>" : ">", 
                    node->data.redirect.out_file);
            }
            if (node->data.redirect.err_file != NULL) {
                printf(" %s %s", 
                   node->data.redirect.append ? "&>>" : "&>", 
                   node->data.redirect.err_file);
            }
            break;
        
        case NODE_AND:
            printf("AND (&&)");
            break;
        case NODE_OR:
            printf("OR (||)");
            break;
        case NODE_SEMICOLON:
            printf("SEMICOLON (;)");
            break;
        case NODE_BACKGROUND:
            printf("BACKGROUND (&)");
            break;
        case NODE_SUBSHELL:
            printf("SUBSHELL");
            break;
        default:
            printf("UNKNOWN");
            break;
    }
    
    printf("\n");
    
    // Рекурсивно печатаем дочерние узлы
    if (node->left != NULL) {
        ast_print(node->left, depth + 1);
    }
    if (node->right != NULL) {
        ast_print(node->right, depth + 1);
    }
}


char **ast_clone_argv(char **argv, int argc) {// Функция для копирования аргументов массива argv изм
    if (argv == NULL || argc <= 0) {
        return NULL;
    }
    
    char **new_argv = (char**)malloc(argc * sizeof(char*));
    if (new_argv == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < argc; i++) {// Копируем каждый аргумент
        new_argv[i] = strdup(argv[i]);
        if (new_argv[i] == NULL) {
            for (int j = 0; j < i; j++) {//освобождаем уже выделенную память в случае ошибки
                free(new_argv[j]);
            }
            free(new_argv);
            return NULL;
        }
    }
    
    return new_argv;
}

//доступ через data.command
int ast_is_command(ast_node_t *node, const char *cmd_name) {// Проверка является ли узел командой с указанным именем изм
    if (node == NULL || node->type != NODE_COMMAND || node->data.command.argc == 0) {
        return 0;
    }
    return strcmp(node->data.command.argv[0], cmd_name) == 0;
}

//доступ через data.command
const char *ast_get_command_name(ast_node_t *node) {// Получение имени команды (первого аргумента) изм
    if (node == NULL || node->type != NODE_COMMAND || node->data.command.argc == 0) {
        return NULL;
    }
    return node->data.command.argv[0];
}
