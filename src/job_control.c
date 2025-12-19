#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include "job_control.h"

//глобальные переменные для управления задачами
static job_t *job_list = NULL;//Список задач
static int next_job_id = 1;//Счетчик ID задач

job_t *create_job(pid_t pgid, const char *command) {// Создает новую задачу
    job_t *job = malloc(sizeof(job_t));
    if (job == NULL) {
        return NULL;
    }
    
    job->job_id = next_job_id++;
    job->pgid = pgid;
    job->command = strdup(command);
    job->state = JOB_RUNNING;
    job->next = NULL;
    
    return job;
}

void add_job(job_t *job) {
    if (job_list == NULL) {
        job_list = job;
    } else {
        // Ищем конец списка и добавляем задачу
        job_t *current = job_list;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = job;
    }
}

// Когда задача завершилась, нужно убрать её из списка, чтобы он не засорялся. Ищет задачу с нужным номером и удаляет её
void remove_job(int job_id) {// Удаляет задачу по ID
    job_t *current = job_list;
    job_t *prev = NULL;
    
    while (current != NULL) {// Ищем задачу в списке
        if (current->job_id == job_id) {
            if (prev != NULL) {// Удаляем задачу из списка
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

//Когда система сообщает, что что-то случилось с процессом (группой), функция ищет какая это была задача в нашем списке
job_t *find_job(pid_t pgid) {// Ищет задачу по ID группы процессов
    job_t *current = job_list;
    while (current != NULL) {
        if (current->pgid == pgid) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Меняет статус задачи. Например, если процесс остановился (Ctrl+Z), меняет "Running" на "Stopped"
void update_job_status(pid_t pgid, job_state_t state) {
    job_t *job = find_job(pgid);
    if (job != NULL) {
        job->state = state;
    }
}

void print_jobs(void) {// Выводит список всех задачю Выводит на экран все запущенные и остановленные задачи с их номерами и статусами
    job_t *current = job_list;
    
    if (current == NULL) {
        printf("Нет активных задач\n");
        return;
    }
    
    while (current != NULL) {
        const char *state_str;
        switch (current->state) {
            case JOB_RUNNING:
                state_str = "Running";
                break;
            case JOB_STOPPED:
                state_str = "Stopped";
                break;
            case JOB_DONE:
                state_str = "Done";
                break;
            default:
                state_str = "Unknown";
        }
        
        printf("[%d] %d %s %s\n", 
               current->job_id, current->pgid, state_str, current->command);
        current = current->next;
    }
}

//Когда вы пишете fg 2, эта функция находит задачу номер 2 в списке
job_t *get_job_by_id(int job_id) {//Ищет задачу по её номеру
    job_t *current = job_list;
    while (current != NULL) {
        if (current->job_id == job_id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}


void sigchld_handler(int sig) {//Обрабатывает сигналы от дочерних процессов
    (void)sig;
    
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {// Проверяем все завершившиеся процессы
        job_t *job = find_job(pid);
        if (job != NULL) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                
                job->state = JOB_DONE;// Процесс завершился
                printf("\n[%d] Done %s\n", job->job_id, job->command);
            } else if (WIFSTOPPED(status)) {
                
                job->state = JOB_STOPPED;// помечает как Процесс остановлен
                printf("\n[%d] Stopped %s\n", job->job_id, job->command);
            }
        }
    }
}

//встроенные команды

int builtin_jobs(char **argv) {//jobs - вывод списка задач
    (void)argv;
    print_jobs();//Когда вводим jobs в shell, вызывается эта функция, которая просто вызывает print_jobs()
    return 0;
}

int builtin_fg(char **argv) {//fg - перевод задачи на передний план
    if (argv[1] == NULL) {
        fprintf(stderr, "fg: использование: fg <job_id>\n");
        return 1;
    }
    
    int job_id = atoi(argv[1]);
    job_t *job = get_job_by_id(job_id);
    if (job == NULL) {
        fprintf(stderr, "fg: задача не найдена: %d\n", job_id);
        return 1;
    }
    
    // Переводим задачу на передний план
    tcsetpgrp(STDIN_FILENO, job->pgid);
    kill(-job->pgid, SIGCONT);
    job->state = JOB_RUNNING;
    
    // Ждем завершения
    int status;
    waitpid(job->pgid, &status, WUNTRACED);
    
    // Возвращаем управление shell
    tcsetpgrp(STDIN_FILENO, getpgrp());
    
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        remove_job(job_id);
    } 
    
    return 0;
}


int builtin_bg(char **argv) {//bg - продолжение задачи в фоне
    if (argv[1] == NULL) {
        fprintf(stderr, "bg: использование: bg <job_id>\n");
        return 1;
    }
    
    int job_id = atoi(argv[1]);
    job_t *job = get_job_by_id(job_id);
    if (job == NULL) {
        fprintf(stderr, "bg: задача не найдена: %d\n", job_id);
        return 1;
    }
    
    
    kill(-job->pgid, SIGCONT);// Продолжаем выполнение задачи
    job->state = JOB_RUNNING;
    printf("[%d] %s\n", job_id, job->command);
    
    return 0;
}


int builtin_kill(char **argv) {//Команда kill - завершение задачи
    if (argv[1] == NULL) {
        fprintf(stderr, "kill: использование: kill <job_id>\n");
        return 1;
    }
    
    int job_id = atoi(argv[1]);
    job_t *job = get_job_by_id(job_id);
    if (job == NULL) {
        fprintf(stderr, "kill: задача не найдена: %d\n", job_id);
        return 1;
    }

    kill(-job->pgid, SIGTERM);
    printf("Сигнал TERM отправлен задаче [%d]\n", job_id);
    return 0;
}
