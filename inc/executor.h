#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "ast.h"

#define MAX_ARGS 256
#define READ_END 0
#define WRITE_END 1
#define MAX_JOBS 100

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} job_state_t;

typedef struct job_t {
    int job_id;
    pid_t pgid;
    char *command;
    job_state_t state;
    struct job_t *next;
} job_t;

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


int execute_ast(ast_node_t *node);
int execute_command(ast_node_t *node, exec_context_t *context);

int execute_simple_command(ast_node_t *node, exec_context_t *context);
int execute_pipeline(ast_node_t *node, exec_context_t *context);
int execute_redirect(ast_node_t *node, exec_context_t *context);
int execute_and(ast_node_t *node, exec_context_t *context);
int execute_or(ast_node_t *node, exec_context_t *context);
int execute_sequence(ast_node_t *node, exec_context_t *context);
int execute_background(ast_node_t *node, exec_context_t *context);

exec_context_t *create_exec_context(void);
void free_exec_context(exec_context_t *context);
void setup_redirections(exec_context_t *context);
int launch_process(char **argv, exec_context_t *context);
int handle_builtin(char **argv);
int is_builtin_command(char *command);

int builtin_cd(char **argv);
int builtin_pwd(char **argv);
int builtin_echo(char **argv);
int builtin_exit(char **argv);
int builtin_help(char **argv);

job_t *create_job(pid_t pgid, const char *command);
void add_job(job_t *job);
void remove_job(int job_id);
job_t *find_job(pid_t pgid);
void update_job_status(pid_t pgid, job_state_t state);
void print_jobs(void);
job_t *get_job_by_id(int job_id);

int builtin_jobs(char **argv);
int builtin_fg(char **argv);
int builtin_bg(char **argv);
int builtin_kill(char **argv);

void setup_signal_handlers(void);
void sigchld_handler(int sig);

#endif
