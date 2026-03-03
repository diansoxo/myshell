#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "ast.h"

// Константы для пайпов
#define READ_END 0
#define WRITE_END 1
#define MAX_ARGV 256

// Структура для контекста выполнения
typedef struct {
    int in_fd;//откуда читаем ввод
    int out_fd;// Фд вывода  
    int err_fd;// Фд ошибок
    int background;// Флаг фонового выполнения 1-фоновая 0-нет
    char *redirect_in;// Файл для перенаправления ввода
    char *redirect_out;// Файл для перенаправления вывода
    char *redirect_err;// Файл для перенаправления ошибок
    int append;// Флаг добавления в файл (>>)
    int in_pipe;// ДОБАВИЛА: Флаг выполнения в пайпе
    pid_t pipeline_pgid; // ДОБАВИЛА ID группы процессов для пайпа
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
int execute_subshell(ast_node_t *node, exec_context_t *context);  // ДОБАВИЛА для подстановок

exec_context_t *create_exec_context(void);// Вспомогательные функции
void free_exec_context(exec_context_t *context);
void setup_redirections(exec_context_t *context);
int launch_process(char **argv, exec_context_t *context);  //ДОБАВИЛА всегда создает форк

// Обработка сигналов
void setup_signal_handlers(void);

#endif
