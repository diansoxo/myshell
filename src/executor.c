#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <termios.h>
#include "executor.h"
#include "builtins.h"

// Глобальные переменные для управления задачами
static job_t *job_list = NULL;    // Список всех задач
static int next_job_id = 1;       // Счетчик для номеров задач

// ========== БАЗОВЫЕ ФУНКЦИИ ==========

// Создаем контекст выполнения - настройки для команды
exec_context_t *create_exec_context(void) {
    exec_context_t *context = (exec_context_t*)malloc(sizeof(exec_context_t));
    if (context == NULL) return NULL;
    
    // Устанавливаем значения по умолчанию
    context->in_fd = STDIN_FILENO;   // Ввод с клавиатуры
    context->out_fd = STDOUT_FILENO; // Вывод на экран  
    context->err_fd = STDERR_FILENO; // Ошибки на экран
    context->background = 0;         // Не фоновая задача
    context->redirect_in = NULL;     // Нет перенаправления ввода
    context->redirect_out = NULL;    // Нет перенаправления вывода
    context->redirect_err = NULL;    // Нет перенаправления ошибок
    context->append = 0;             // Обычная перезапись файлов
    
    return context;
}

// Освобождаем память контекста
void free_exec_context(exec_context_t *context) {
    if (context == NULL) return;
    
    // Освобождаем строки с именами файлов
    if (context->redirect_in != NULL) free(context->redirect_in);
    if (context->redirect_out != NULL) free(context->redirect_out);
    if (context->redirect_err != NULL) free(context->redirect_err);
    
    free(context);
}

// Настраиваем перенаправления ввода/вывода
void setup_redirections(exec_context_t *context) {
    // Перенаправляем ввод из файла
    if (context->redirect_in != NULL) {
        int fd = open(context->redirect_in, O_RDONLY);
        if (fd < 0) {
            perror("open input file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);  // Заменяем стандартный ввод
        close(fd);
    }
    
    // Перенаправляем вывод в файл
    if (context->redirect_out != NULL) {
        int flags = O_WRONLY | O_CREAT;
        // Выбираем режим: добавление или перезапись
        flags |= context->append ? O_APPEND : O_TRUNC;
        
        int fd = open(context->redirect_out, flags, 0644);
        if (fd < 0) {
            perror("open output file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDOUT_FILENO);  // Заменяем стандартный вывод
        close(fd);
    }
    
    // Перенаправляем ошибки в файл
    if (context->redirect_err != NULL) {
        int flags = O_WRONLY | O_CREAT;
        flags |= context->append ? O_APPEND : O_TRUNC;
        
        int fd = open(context->redirect_err, flags, 0644);
        if (fd < 0) {
            perror("open error file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDERR_FILENO);  // Заменяем стандартный вывод ошибок
        close(fd);
    }
}

// ========== ВЫПОЛНЕНИЕ AST ДЕРЕВА ==========

// Главная функция выполнения дерева команд
int execute_ast(ast_node_t *node) {
    // Создаем контекст выполнения
    exec_context_t *context = create_exec_context();
    if (context == NULL) {
        fprintf(stderr, "Ошибка создания контекста выполнения\n");
        return -1;
    }
    
    // Выполняем команду
    int result = execute_command(node, context);
    
    // Освобождаем память
    free_exec_context(context);
    return result;
}

// Выполняем команду в зависимости от типа узла
int execute_command(ast_node_t *node, exec_context_t *context) {
    if (node == NULL) return 0;
    
    // В зависимости от типа команды вызываем нужную функцию
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
            fprintf(stderr, "Неизвестный тип узла AST\n");
            return -1;
    }
}

// ========== ФУНКЦИИ ДЛЯ РАЗНЫХ ТИПОВ КОМАНД ==========

// Выполняем простую команду (например: ls -l)
int execute_simple_command(ast_node_t *node, exec_context_t *context) {
    if (node == NULL || node->argv == NULL || node->argc == 0) {
        return 0;  // Пустая команда - ничего не делаем
    }
    
    // Проверяем встроенная ли это команда (cd, echo и т.д.)
    if (is_builtin_command(node->argv[0])) {
        return handle_builtin(node->argv);
    }
    
    // Настраиваем перенаправления из команды
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
    
    // Запускаем внешнюю программу
    return launch_process(node->argv, context);
}

// Выполняем конвейер (команда1 | команда2)
int execute_pipeline(ast_node_t *node, exec_context_t *context) {
    int pipefd[2];
    
    // Создаем pipe - канал связи между процессами
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }
    
    // Сохраняем оригинальные настройки ввода/вывода
    int saved_stdout = dup(STDOUT_FILENO);
    int saved_stdin = dup(STDIN_FILENO);
    
    // Настраиваем pipe для левой команды (ее вывод идет в pipe)
    dup2(pipefd[WRITE_END], STDOUT_FILENO);
    close(pipefd[WRITE_END]);
    
    // Выполняем левую команду (ее вывод пойдет в pipe)
    execute_command(node->left, context);

    // Восстанавливаем стандартный вывод
    dup2(saved_stdout, STDOUT_FILENO);
    
    // Настраиваем ввод для правой команды (читает из pipe)
    dup2(pipefd[READ_END], STDIN_FILENO);
    close(pipefd[READ_END]);
    
    // Выполняем правую команду (читает из pipe)
    int right_status = execute_command(node->right, context);
    
    // Восстанавливаем стандартный ввод
    dup2(saved_stdin, STDIN_FILENO);
    
    // Закрываем сохраненные дескрипторы
    close(saved_stdout);
    close(saved_stdin);
    
    return right_status;
}

// Выполняем перенаправления (команда > файл)
int execute_redirect(ast_node_t *node, exec_context_t *context) {
    if (node == NULL) return 0;
    
    // Создаем копию контекста для перенаправлений
    exec_context_t *redirect_context = create_exec_context();
    if (redirect_context == NULL) return -1;
    
    // Копируем настройки из основного контекста
    redirect_context->in_fd = context->in_fd;
    redirect_context->out_fd = context->out_fd;
    redirect_context->err_fd = context->err_fd;
    redirect_context->background = context->background;
    
    // Устанавливаем перенаправления из узла AST
    if (node->in_file != NULL) {
        redirect_context->redirect_in = strdup(node->in_file);
    }
    if (node->out_file != NULL) {
        redirect_context->redirect_out = strdup(node->out_file);
        redirect_context->append = node->append;
    }
    if (node->err_file != NULL) {
        redirect_context->redirect_err = strdup(node->err_file);
        redirect_context->append = node->append;
    }
    
    // Выполняем команду с перенаправлениями
    int result = execute_command(node->left, redirect_context);
    
    // Освобождаем память
    free_exec_context(redirect_context);
    return result;
}

// Выполняем И (команда1 && команда2)
int execute_and(ast_node_t *node, exec_context_t *context) {
    // Выполняем левую команду
    int left_status = execute_command(node->left, context);
    
    // Если первая команда успешна (вернула 0), выполняем вторую
    if (left_status == 0) {
        return execute_command(node->right, context);
    }
    
    // Иначе возвращаем статус первой команды
    return left_status;
}

// Выполняем ИЛИ (команда1 || команда2)
int execute_or(ast_node_t *node, exec_context_t *context) {
    // Выполняем левую команду
    int left_status = execute_command(node->left, context);
    
    // Если первая команда неуспешна (вернула не 0), выполняем вторую
    if (left_status != 0) {
        return execute_command(node->right, context);
    }
    
    // Иначе возвращаем статус первой команды
    return left_status;
}

// Выполняем последовательность (команда1 ; команда2)
int execute_sequence(ast_node_t *node, exec_context_t *context) {
    // Выполняем левую команду
    execute_command(node->left, context);
    
    // Выполняем правую команду
    return execute_command(node->right, context);
}

// Выполняем в фоне (команда &)
int execute_background(ast_node_t *node, exec_context_t *context) {
    context->background = 1;  // Устанавливаем фоновый режим
    return execute_command(node->left, context);
}

// ========== ЗАПУСК ВНЕШНИХ ПРОГРАММ ==========

// Запускаем внешнюю программу
int launch_process(char **argv, exec_context_t *context) {
    pid_t pid = fork();  // Создаем новый процесс
    
    if (pid == 0) {
        // Это дочерний процесс (где выполняется команда)
        
        // Настраиваем группу процессов
        if (context->background) {
            setpgid(0, 0);  // Создаем новую группу для фоновых
        } else {
            setpgid(0, getpgid(0));
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
            tcsetpgrp(STDIN_FILENO, pid);  // Отдаем управление
            
            int status;
            waitpid(pid, &status, WUNTRACED);  // Ждем завершения
            
            tcsetpgrp(STDIN_FILENO, getpgrp());  // Возвращаем управление
            
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
        }
    }
    return 0;
}

// ========== УПРАВЛЕНИЕ ЗАДАЧАМИ ==========

// Создаем новую задачу
job_t *create_job(pid_t pgid, const char *command) {
    job_t *job = (job_t*)malloc(sizeof(job_t));
    if (!job) return NULL;
    
    job->job_id = next_job_id++;
    job->pgid = pgid;
    job->command = strdup(command);
    job->state = JOB_RUNNING;
    job->next = NULL;
    
    return job;
}

// Добавляем задачу в список
void add_job(job_t *job) {
    if (!job_list) {
        job_list = job;  // Первая задача в списке
    } else {
        // Идем до конца списка и добавляем
        job_t *current = job_list;
        while (current->next) {
            current = current->next;
        }
        current->next = job;
    }
}

// Удаляем задачу из списка
void remove_job(int job_id) {
    job_t *current = job_list;
    job_t *prev = NULL;
    
    // Ищем задачу в списке
    while (current) {
        if (current->job_id == job_id) {
            // Нашли - удаляем из списка
            if (prev) {
                prev->next = current->next;
            } else {
                job_list = current->next;
            }
            free(current->command);
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

// Ищем задачу по ID группы процессов
job_t *find_job(pid_t pgid) {
    job_t *current = job_list;
    while (current) {
        if (current->pgid == pgid) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Обновляем статус задачи
void update_job_status(pid_t pgid, job_state_t state) {
    job_t *job = find_job(pgid);
    if (job) {
        job->state = state;
    }
}

// Печатаем список всех задач
void print_jobs(void) {
    job_t *current = job_list;
    if (!current) {
        printf("Нет активных задач\n");
        return;
    }
    
    while (current) {
        // Преобразуем состояние в строку
        const char *state_str = "Running";
        if (current->state == JOB_STOPPED) state_str = "Stopped";
        else if (current->state == JOB_DONE) state_str = "Done";
        
        printf("[%d] %d %s %s\n", 
               current->job_id, current->pgid, state_str, current->command);
        current = current->next;
    }
}

// Ищем задачу по номеру
job_t *get_job_by_id(int job_id) {
    job_t *current = job_list;
    while (current) {
        if (current->job_id == job_id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// ========== ОБРАБОТКА СИГНАЛОВ ==========

// Обработчик для завершения дочерних процессов
void sigchld_handler(int sig) {
    (void)sig; // Игнорируем параметр
    int status;
    pid_t pid;
    
    // Проверяем все завершившиеся процессы
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        job_t *job = find_job(pid);
        if (job) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                // Процесс завершился
                job->state = JOB_DONE;
                printf("\n[%d] Done %s\n", job->job_id, job->command);
            } else if (WIFSTOPPED(status)) {
                // Процесс остановлен
                job->state = JOB_STOPPED;
                printf("\n[%d] Stopped %s\n", job->job_id, job->command);
            }
        }
    }
}

// Настраиваем обработчики сигналов
void setup_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
    
    // Игнорируем Ctrl+C и Ctrl+\ в shell
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
}

// ========== ВСТРОЕННЫЕ КОМАНДЫ ДЛЯ УПРАВЛЕНИЯ ЗАДАЧАМИ ==========

// Команда jobs - показать все задачи
int builtin_jobs(char **argv) {
    (void)argv; // Игнорируем аргументы
    print_jobs();
    return 0;
}

// Команда fg - перевести задачу на передний план
int builtin_fg(char **argv) {
    if (!argv[1]) {
        fprintf(stderr, "fg: usage: fg <job_id>\n");
        return 1;
    }
    
    int job_id = atoi(argv[1]);
    job_t *job = get_job_by_id(job_id);
    if (!job) {
        fprintf(stderr, "fg: job not found: %d\n", job_id);
        return 1;
    }
    
    // Переводим задачу на передний план
    tcsetpgrp(STDIN_FILENO, job->pgid);
    kill(-job->pgid, SIGCONT);  // Продолжаем выполнение
    job->state = JOB_RUNNING;
    
    // Ждем завершения
    int status;
    waitpid(job->pgid, &status, WUNTRACED);
    
    // Возвращаем управление shell
    tcsetpgrp(STDIN_FILENO, getpgrp());
    
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        remove_job(job_id);  // Удаляем если завершилась
    }
    
    return 0;
}

// Команда bg - продолжить задачу в фоне
int builtin_bg(char **argv) {
    if (!argv[1]) {
        fprintf(stderr, "bg: usage: bg <job_id>\n");
        return 1;
    }
    
    int job_id = atoi(argv[1]);
    job_t *job = get_job_by_id(job_id);
    if (!job) {
        fprintf(stderr, "bg: job not found: %d\n", job_id);
        return 1;
    }
    
    kill(-job->pgid, SIGCONT);  // Продолжаем выполнение
    job->state = JOB_RUNNING;
    printf("[%d] %s\n", job_id, job->command);
    
    return 0;
}

// Команда kill - завершить задачу
int builtin_kill(char **argv) {
    if (!argv[1]) {
        fprintf(stderr, "kill: usage: kill <job_id>\n");
        return 1;
    }
    
    int job_id = atoi(argv[1]);
    job_t *job = get_job_by_id(job_id);
    if (!job) {
        fprintf(stderr, "kill: job not found: %d\n", job_id);
        return 1;
    }
    
    kill(-job->pgid, SIGTERM);  // Отправляем сигнал завершения
    printf("Сигнал TERM отправлен задаче [%d]\n", job_id);
    return 0;
}
