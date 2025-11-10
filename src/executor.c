#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "executor.h"

// Создание контекста выполнения
exec_context_t *create_exec_context(void) {
    exec_context_t *context = (exec_context_t*)malloc(sizeof(exec_context_t));
    if (context == NULL) return NULL;
    
    context->in_fd = STDIN_FILENO;
    context->out_fd = STDOUT_FILENO;
    context->err_fd = STDERR_FILENO;
    context->background = 0;
    context->redirect_in = NULL;
    context->redirect_out = NULL;
    context->redirect_err = NULL;
    context->append = 0;
    
    return context;
}

void free_exec_context(exec_context_t *context) {
    if (context == NULL) return;
    
    if (context->redirect_in != NULL) free(context->redirect_in);
    if (context->redirect_out != NULL) free(context->redirect_out);
    if (context->redirect_err != NULL) free(context->redirect_err);
    
    free(context);
}

// Настройка перенаправлений
void setup_redirections(exec_context_t *context) {
    if (context->redirect_in != NULL) {
        int fd = open(context->redirect_in, O_RDONLY);
        if (fd < 0) {
            perror("open input file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    
    if (context->redirect_out != NULL) {
        int flags = O_WRONLY | O_CREAT;
        flags |= context->append ? O_APPEND : O_TRUNC;
        
        int fd = open(context->redirect_out, flags, 0644);
        if (fd < 0) {
            perror("open output file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    
    if (context->redirect_err != NULL) {
        int flags = O_WRONLY | O_CREAT;
        flags |= context->append ? O_APPEND : O_TRUNC;
        
        int fd = open(context->redirect_err, flags, 0644);
        if (fd < 0) {
            perror("open error file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDERR_FILENO);
        close(fd);
    }
}

// Основная функция выполнения AST
int execute_ast(ast_node_t *node) {
    exec_context_t *context = create_exec_context();
    if (context == NULL) {
        fprintf(stderr, "Ошибка создания контекста выполнения\n");
        return -1;
    }
    
    int result = execute_command(node, context);
    free_exec_context(context);
    return result;
}

// Выполнение команды в зависимости от типа узла
int execute_command(ast_node_t *node, exec_context_t *context) {
    if (node == NULL) return 0;
    
    switch (node->type) {
        case NODE_COMMAND:
            return execute_simple_command(node, context);
        case NODE_PIPE:
            return execute_pipeline(node, context);
        case NODE_REDIRECT:
            return execute_redirect(node, context);
        case NODE_AND:
            return execute_and(node, context);
        case NODE_OR:
            return execute_or(node, context);
        case NODE_SEMICOLON:
            return execute_sequence(node, context);
        case NODE_BACKGROUND:
            return execute_background(node, context);
        case NODE_SUBSHELL:
            // Для простоты выполняем как обычную команду
            return execute_simple_command(node->left, context);
        default:
            fprintf(stderr, "Неизвестный тип узла AST\n");
            return -1;
    }
}

// Выполнение простой команды
int execute_simple_command(ast_node_t *node, exec_context_t *context) {
    if (node == NULL || node->argv == NULL || node->argc == 0) {
        return 0;
    }
    
    // Проверка встроенных команд
    if (is_builtin_command(node->argv[0])) {
        return handle_builtin(node->argv);
    }
    
    // Настройка перенаправлений из узла команды
    if (node->in_file != NULL) {
        context->redirect_in = strdup(node->in_file);
    }
    if (node->out_file != NULL) {
        context->redirect_out = strdup(node->out_file);
        context->append = node->append;
    }
    if (node->err_file != NULL) {
        context->redirect_err = strdup(node->err_file);
        context->append = node->append;
    }
    
    return launch_process(node->argv, context);
}

// Запуск внешнего процесса
int launch_process(char **args, exec_context_t *context) {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        setup_redirections(context);
        
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
        return -1;
    } else {
        // Родительский процесс
        if (!context->background) {
            int status;
            waitpid(pid, &status, 0);
            return WEXITSTATUS(status);
        } else {
            printf("[%d] %d\n", 1, pid); // Простой вывод для фоновой задачи
            return 0;
        }
    }
}

// Выполнение конвейера
int execute_pipeline(ast_node_t *node, exec_context_t *context) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }
    
    // Сохраняем оригинальные файловые дескрипторы
    int saved_stdout = dup(STDOUT_FILENO);
    int saved_stdin = dup(STDIN_FILENO);
    
    // Настраиваем pipe для левой команды (выход в pipe)
    dup2(pipefd[WRITE_END], STDOUT_FILENO);
    close(pipefd[WRITE_END]);
    
    int left_status = execute_command(node->left, context);
    
    // Восстанавливаем stdout и настраиваем stdin для правой команды
    dup2(saved_stdout, STDOUT_FILENO);
    dup2(pipefd[READ_END], STDIN_FILENO);
    close(pipefd[READ_END]);
    
    int right_status = execute_command(node->right, context);
    
    // Восстанавливаем оригинальные дескрипторы
    dup2(saved_stdin, STDIN_FILENO);
    close(saved_stdout);
    close(saved_stdin);
    
    return right_status; // Возвращаем статус последней команды в конвейере
}

// Остальные функции выполнения (упрощенные версии)
int execute_redirect(ast_node_t *node, exec_context_t *context) {
    // Для простоты считаем, что перенаправления обрабатываются в командах
    return execute_command(node->left, context);
}

int execute_and(ast_node_t *node, exec_context_t *context) {
    int left_status = execute_command(node->left, context);
    if (left_status == 0) {
        return execute_command(node->right, context);
    }
    return left_status;
}

int execute_or(ast_node_t *node, exec_context_t *context) {
    int left_status = execute_command(node->left, context);
    if (left_status != 0) {
        return execute_command(node->right, context);
    }
    return left_status;
}

int execute_sequence(ast_node_t *node, exec_context_t *context) {
    execute_command(node->left, context);
    return execute_command(node->right, context);
}

int execute_background(ast_node_t *node, exec_context_t *context) {
    context->background = 1;
    return execute_command(node->left, context);
}

// Проверка встроенных команд
int is_builtin_command(char *command) {
    if (command == NULL) return 0;
    
    char *builtins[] = {"cd", "pwd", "echo", "exit", "help", NULL};
    
    for (int i = 0; builtins[i] != NULL; i++) {
        if (strcmp(command, builtins[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Обработка встроенных команд
int handle_builtin(char **args) {
    if (args[0] == NULL) return 0;
    
    if (strcmp(args[0], "cd") == 0) {
        return builtin_cd(args);
    } else if (strcmp(args[0], "pwd") == 0) {
        return builtin_pwd(args);
    } else if (strcmp(args[0], "echo") == 0) {
        return builtin_echo(args);
    } else if (strcmp(args[0], "exit") == 0) {
        return builtin_exit(args);
    } else if (strcmp(args[0], "help") == 0) {
        return builtin_help(args);
    }
    
    return 0;
}

// Реализации встроенных команд
int builtin_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "cd: ожидается аргумент\n");
        return 1;
    }
    
    if (chdir(args[1]) != 0) {
        perror("cd");
        return 1;
    }
    return 0;
}

int builtin_pwd(char **args) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    } else {
        perror("pwd");
        return 1;
    }
}

int builtin_echo(char **args) {
    for (int i = 1; args[i] != NULL; i++) {
        printf("%s", args[i]);
        if (args[i + 1] != NULL) {
            printf(" ");
        }
    }
    printf("\n");
    return 0;
}

int builtin_exit(char **args) {
    int exit_code = 0;
    if (args[1] != NULL) {
        exit_code = atoi(args[1]);
    }
    exit(exit_code);
}

int builtin_help(char **args) {
    printf("Встроенные команды:\n");
    printf("cd [dir]- сменить директорию\n");
    printf("pwd- показать текущую директорию\n");
    printf("echo [text]- вывести текст\n");
    printf("exit [code]- выйти из shell\n");
    printf("help- показать эту справку\n");
    printf("Поддерживаются перенаправления (> >> <) и конвейеры (|)\n");
    return 0;
}