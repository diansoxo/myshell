#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "ast.h"

#define MAX_ARGS 256
#define READ_END 0
#define WRITE_END 1

typedef struct {
    int in_fd;          // stdin file descriptor
    int out_fd;         // stdout file descriptor  
    int err_fd;         // stderr file descriptor
    int background;     // 1 if command should run in background
    char *redirect_in;  // filename for input redirection
    char *redirect_out; // filename for output redirection
    char *redirect_err; // filename for error redirection
    int append;         // 1 for append mode (>>)
} exec_context_t;

// Main execution functions
int execute_ast(ast_node_t *node);
int execute_command(ast_node_t *node, exec_context_t *context);

// Command type handlers
int execute_simple_command(ast_node_t *node, exec_context_t *context);
int execute_pipeline(ast_node_t *node, exec_context_t *context);
int execute_redirect(ast_node_t *node, exec_context_t *context);
int execute_and(ast_node_t *node, exec_context_t *context);
int execute_or(ast_node_t *node, exec_context_t *context);
int execute_sequence(ast_node_t *node, exec_context_t *context);
int execute_background(ast_node_t *node, exec_context_t *context);

// Helper functions
exec_context_t *create_exec_context(void);
void free_exec_context(exec_context_t *context);
void setup_redirections(exec_context_t *context);
void restore_redirections(exec_context_t *context);
int launch_process(char **args, exec_context_t *context);
int handle_builtin(char **args);
int is_builtin_command(char *command);

// Builtin commands
int builtin_cd(char **args);
int builtin_pwd(char **args);
int builtin_echo(char **args);
int builtin_exit(char **args);
int builtin_help(char **args);

#endif