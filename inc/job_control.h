#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <sys/types.h>

#define MAX_JOBS 100

// Состояния задач
typedef enum {
    JOB_RUNNING,  // Задача выполняется
    JOB_STOPPED,  // Задача остановлена
    JOB_DONE      // Задача завершена
} job_state_t;

// Структура задачи
typedef struct job_t {
    int job_id;           // ID задачи
    pid_t pgid;           // ID группы процессов
    char *command;        // Команда задачи
    job_state_t state;    // Состояние задачи
    struct job_t *next;   // Следующая задача в списке
} job_t;

// Управление задачами
job_t *create_job(pid_t pgid, const char *command);
void add_job(job_t *job);
void remove_job(int job_id);
job_t *find_job(pid_t pgid);
void update_job_status(pid_t pgid, job_state_t state);
void print_jobs(void);
job_t *get_job_by_id(int job_id);

// Встроенные команды управления задачами
int builtin_jobs(char **argv);
int builtin_fg(char **argv);
int builtin_bg(char **argv);
int builtin_kill(char **argv);

// Обработчики сигналов
void sigchld_handler(int sig);

#endif
