#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "ast.h"

#define READ_END 0
#define WRITE_END 1
#define MAX_ARGV 256

typedef struct {
    int in_fd;
    int out_fd;
    int err_fd;
    int background;
    char *redirect_in;
    char *redirect_out;
    char *redirect_err;
    int append;
} exec_context_t;


int execute_ast(ast_node_t *node);// Основные функции выполнения
int execute_command(ast_node_t *node, exec_context_t *context);


int execute_simple_command(ast_node_t *node, exec_context_t *context);// Обработчики типов команд
int execute_pipeline(ast_node_t *node, exec_context_t *context);
int execute_redirect(ast_node_t *node, exec_context_t *context);
int execute_and(ast_node_t *node, exec_context_t *context);
int execute_or(ast_node_t *node, exec_context_t *context);
int execute_sequence(ast_node_t *node, exec_context_t *context);
int execute_background(ast_node_t *node, exec_context_t *context);


exec_context_t *create_exec_context(void);// Вспомогательные функции
void free_exec_context(exec_context_t *context);
void setup_redirections(exec_context_t *context);
int launch_process(char **argv, exec_context_t *context);


void setup_signal_handlers(void);// Обработка сигналов

#endif
