#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "executor.h"
#include "builtins.h"
#include "job_control.h"

exec_context_t *create_exec_context(void) {//инициализирует контекст выполнения команды
    exec_context_t *context = malloc(sizeof(exec_context_t));
    if (context == NULL) {
        return NULL;
    }
    
    context->in_fd = STDIN_FILENO;//куда читать ввод
    context->out_fd = STDOUT_FILENO;//куда писать вывод
    context->err_fd = STDERR_FILENO;//куда писать ошибки
    context->background = 0;//в фоне или нет
    context->redirect_in = NULL;//имя фалй для перенаправления ввода
    context->redirect_out = NULL;
    context->redirect_err = NULL;
    context->append = 0;//добавление в файл или перезапись
    
    return context;
}

void free_exec_context(exec_context_t *context) {// Освобождает память контекста выполнения
    if (context == NULL) {
        return;
    }
    
    if (context->redirect_in != NULL) {// Освобождаем строки с именами файлов
        free(context->redirect_in);
    }
    if (context->redirect_out != NULL) {
        free(context->redirect_out);
    }
    if (context->redirect_err != NULL) {
        free(context->redirect_err);
    }
    
    free(context);
}

void setup_redirections(exec_context_t *context) {
    if (context->redirect_in != NULL) {// Перенаправление ввода из файла
        if (context->redirect_in != NULL) {
        int fd = open(context->redirect_in, O_RDONLY);
        if (fd < 0) {
            perror("open input file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    }
    if (context->redirect_out != NULL) {// Перенаправление вывода в файл
        int flags = O_WRONLY | O_CREAT;
        if (context->append) {
            flags |= O_APPEND;  // Добавление в конец файла
        } else {
            flags |= O_TRUNC;   // Перезапись файла
        }
        
        int fd = open(context->redirect_out, flags, 0644);
        if (fd < 0) {
            perror("open output file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    
    if (context->redirect_err != NULL) {// Перенаправление ошибок в файл
        int flags = O_WRONLY | O_CREAT;
        if (context->append) {
            flags |= O_APPEND;
        } else {
            flags |= O_TRUNC;
        }
        
        int fd = open(context->redirect_err, flags, 0644);
        if (fd < 0) {
            perror("open error file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDERR_FILENO);
        close(fd);
    }
}

int execute_ast(ast_node_t *node) {// Основная функция для выполнения AST дерева
    exec_context_t *context = create_exec_context();
    if (context == NULL) {
        fprintf(stderr, "Ошибка: не удалось создать контекст выполнения\n");
        return -1;
    }
    
    int result = execute_command(node, context);
    free_exec_context(context);
    return result;
}


int execute_command(ast_node_t *node, exec_context_t *context) {// Выполняет команду в зависимости от типа узла AST
    if (node == NULL) {
        return 0;
    }
    
    // Выбор функции выполнения в зависимости от типа узла
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
            return execute_simple_command(node->left, context);
        default:
            fprintf(stderr, "Ошибка: неизвестный тип узла AST\n");
            return -1;
    }
}


int execute_simple_command(ast_node_t *node, exec_context_t *context) {//вып прост команд
    
    if (node == NULL || node->type != NODE_COMMAND || node->data.command.argv == NULL || node->data.command.argc == 0) {// Проверка на пустую команду
        return 0;
    }
    
    if (is_builtin_command(node->data.command.argv[0])) {// Проверяем встроенную команду
        return handle_builtin(node->data.command.argv);
    }//Теперь перенаправления хранятся в отдельном узле NODE_REDIRECT команда больше не содержит in_file, out_file, err_file эти поля теперь в узле NODE_REDIRECT
    
    
    return launch_process(node->data.command.argv, context);//запускаем внешнюю программу
}


int execute_pipeline(ast_node_t *node, exec_context_t *context) {// Выполняет конвейер команд
    int pipefd[2];
    
    if (pipe(pipefd) == -1) {// Создаем пайп
        perror("pipe");
        return -1;
    }
    
    int saved_stdout = dup(STDOUT_FILENO);// Сохраняем оригинальные дескрипторы
    int saved_stdin = dup(STDIN_FILENO);
    
    int redirect_stderr = (node->type == NODE_PIPE && node->data.pipe.redirect_err);//изм проверяем флаг redirect_err
    
    
    dup2(pipefd[WRITE_END], STDOUT_FILENO);// Настраиваем вывод левой команды на пайп
    close(pipefd[WRITE_END]);
    
    // Если нужно перенаправить stderr
    if (redirect_stderr) {
        dup2(pipefd[WRITE_END], STDERR_FILENO);
    }
    
    // Выполняем левую команду (ее вывод пойдет в пайп)
    execute_command(node->left, context);

    // Восстанавливаем стандартный вывод
    dup2(saved_stdout, STDOUT_FILENO);
    
    // Настраиваем ввод правой команды из пайпа
    dup2(pipefd[READ_END], STDIN_FILENO);
    close(pipefd[READ_END]);
    
    // Выполняем правую команду (читает из пайпа)
    int right_status = execute_command(node->right, context);
    
    // Восстанавливаем стандартный ввод
    dup2(saved_stdin, STDIN_FILENO);
    
    // Закрываем сохраненные дескрипторы
    close(saved_stdout);
    close(saved_stdin);
    
    return right_status;
}


int execute_redirect(ast_node_t *node, exec_context_t *context) {// Выполняет перенаправления ввода/вывода/ошибок
    if (node == NULL || node->type != NODE_REDIRECT) {
        return 0;
    }
    
    exec_context_t *redirect_context = create_exec_context();// Создаем новый контекст для перенаправлений
    if (redirect_context == NULL) {
        return -1;
    }
    
    redirect_context->in_fd = context->in_fd;
    redirect_context->out_fd = context->out_fd;
    redirect_context->err_fd = context->err_fd;
    redirect_context->background = context->background;
    
    
    if (node->data.redirect.in_file != NULL) {// Устанавливаем перенаправления из узла
        redirect_context->redirect_in = strdup(node->data.redirect.in_file);//изм
    }
    if (node->data.redirect.out_file != NULL) {
        redirect_context->redirect_out = strdup(node->data.redirect.out_file);//изм
        redirect_context->append = node->data.redirect.append;//изм
    }
    if (node->data.redirect.err_file != NULL) {
        redirect_context->redirect_err = strdup(node->data.redirect.err_file);//изм
        redirect_context->append = node->data.redirect.append;//изм
    }
    
    int result = execute_command(node->left, redirect_context);//выполняем команду с перенаправлениями
    free_exec_context(redirect_context);
    return result;
}

int execute_and(ast_node_t *node, exec_context_t *context) {// Выполняет оператор И (&&)
    // Выполняем левую команду
    int left_status = execute_command(node->left, context);
    
    // Если первая команда успешна, выполняем вторую
    if (left_status == 0) {
        return execute_command(node->right, context);
    }
    
    return left_status;
}

int execute_or(ast_node_t *node, exec_context_t *context) {// Выполняет оператор ИЛИ (||)
    // Выполняем левую команду
    int left_status = execute_command(node->left, context);
    
    // Если первая команда неуспешна, выполняем вторую
    if (left_status != 0) {
        return execute_command(node->right, context);
    }
    
    return left_status;
}


int execute_sequence(ast_node_t *node, exec_context_t *context) {// Выполняет последовательность команд ;

    execute_command(node->left, context);
    
    return execute_command(node->right, context);
}


int execute_background(ast_node_t *node, exec_context_t *context) {// Выполняет команду в фоне (&)
    context->background = 1;
    return execute_command(node->left, context);
}

int launch_process(char **argv, exec_context_t *context) {
    pid_t pid = fork();  // Создаем новый процесс
    
    if (pid == 0) {
        // Это дочерний процесс (где выполняется команда)
        
        // Настраиваем группу процессов
        if (context->background) {
            setpgid(0, 0);  // Создаем новую группу для фоновых
        }
        
        // Восстанавливаем стандартные обработчики сигналов
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        
        // Настраиваем перенаправления
        setup_redirections(context);
        
        // Заменяем этот процесс на новую программу
        execvp(argv[0], argv);
        
        // Если дошли сюда - значит execvp не сработал
        perror("execvp");
        exit(EXIT_FAILURE);
        
    } else if (pid < 0) {
        // Ошибка при создании процесса
        perror("fork");
        return -1;
    } else {
        // Это родительский процесс (наш shell)
        if (context->background) {
            // Фоновая задача
            setpgid(pid, pid);
            
            // Добавляем в список задач
            job_t *job = create_job(pid, argv[0]);
            add_job(job);
            printf("[%d] %d\n", job->job_id, pid);
            return 0;
        } else {
            // Обычная задача - ждем завершения
            setpgid(pid, pid);
            
            int status;
            waitpid(pid, &status, WUNTRACED);  // Ждем завершения
            
            // Обрабатываем результат
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);  // Нормальное завершение
            } else if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);  // Завершение по сигналу
            } else if (WIFSTOPPED(status)) {
                // Задача остановлена - добавляем в список
                job_t *job = create_job(pid, argv[0]);
                job->state = JOB_STOPPED;
                add_job(job);
                printf("[%d] Stopped %s\n", job->job_id, argv[0]);
            }
            
            return 0;
        }
    }
}


void setup_signal_handlers(void) {// Настраивает обработчики сигналов для shell
    struct sigaction sa;
    
    // Настраиваем обработчик SIGCHLD
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);
    
    // Игнорируем сигналы в shell
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
}
