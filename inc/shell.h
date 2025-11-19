#ifndef SHELL_H
#define SHELL_H

typedef struct {
    int running;
    char *prompt;
} shell_t;

shell_t *shell_create(void);
void shell_destroy(shell_t *shell);
void shell_run(shell_t *shell);

#endif

//работает ли она сейчас и объявляет три главные функции
// создать оболочку, запустить ее главный цикл и уничтожить при выходе
