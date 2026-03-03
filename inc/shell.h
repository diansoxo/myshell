#ifndef SHELL_H
#define SHELL_H

typedef struct history_entry {// Структура для истории команд
    char *command;
    struct history_entry *next;
    struct history_entry *prev;
} history_entry_t;

typedef struct {// Структура shell
    int running;
    
    // История команд
    history_entry_t *history_head;
    history_entry_t *history_tail;
    history_entry_t *history_current;
    int history_count;
    char *history_file;
    
    // Для редактирования
    char line_buffer[MAX_LINE_LENGTH];
    int cursor_pos;
    int line_len;
} shell_t;

shell_t *shell_create(void);
void shell_destroy(shell_t *shell);
void shell_run(shell_t *shell);

void history_add(shell_t *shell, const char *command);// Функции для работы с историей
void history_load(shell_t *shell);
void history_save(shell_t *shell);
void history_print(shell_t *shell);
const char *history_prev(shell_t *shell);
const char *history_next(shell_t *shell);

#endif

//работает ли она сейчас и объявляет три главные функции
// создать оболочку, запустить ее главный цикл и уничтожить при выходе
