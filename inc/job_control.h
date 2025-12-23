#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <sys/types.h>

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

void sigchld_handler(int sig);

#endif
