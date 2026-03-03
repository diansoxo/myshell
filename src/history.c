#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shell.h"

void history_add(shell_t *shell, const char *command) {
    if (!command || strlen(command) == 0) return;
    
    
    if (shell->history_tail && strcmp(shell->history_tail->command, command) == 0) {// Не добавляем пустые команды и дубликаты подряд
        return;
    }
    
    history_entry_t *entry = malloc(sizeof(history_entry_t));
    entry->command = strdup(command);
    entry->next = NULL;
    entry->prev = shell->history_tail;
    
    if (shell->history_tail) {
        shell->history_tail->next = entry;
    }
    shell->history_tail = entry;
    
    if (!shell->history_head) {
        shell->history_head = entry;
    }
    
    shell->history_count++;
    
    
    while (shell->history_count > MAX_HISTORY) {// Ограничиваем размер истории
        history_entry_t *old = shell->history_head;
        shell->history_head = old->next;
        if (shell->history_head) {
            shell->history_head->prev = NULL;
        }
        free(old->command);
        free(old);
        shell->history_count--;
    }
    
    shell->history_current = shell->history_tail;
}

void history_load(shell_t *shell) {
    const char *home = getenv("HOME");
    if (!home) return;
    
    char path[1024];
    snprintf(path, sizeof(path), "%s/.my_shell_history", home);
    
    shell->history_file = strdup(path);
    
    FILE *f = fopen(path, "r");
    if (!f) return;
    
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        history_add(shell, line);
    }
    
    fclose(f);
}

void history_save(shell_t *shell) {
    if (!shell->history_file) return;
    
    FILE *f = fopen(shell->history_file, "w");
    if (!f) return;
    
    history_entry_t *entry = shell->history_head;
    while (entry) {
        fprintf(f, "%s\n", entry->command);
        entry = entry->next;
    }
    
    fclose(f);
}

void history_print(shell_t *shell) {
    history_entry_t *entry = shell->history_head;
    int i = 1;
    
    while (entry) {
        printf("%5d  %s\n", i++, entry->command);
        entry = entry->next;
    }
}

const char *history_prev(shell_t *shell) {
    if (!shell->history_current) return NULL;
    
    if (shell->history_current->prev) {
        shell->history_current = shell->history_current->prev;
    }
    
    return shell->history_current->command;
}

const char *history_next(shell_t *shell) {
    if (!shell->history_current) return NULL;
    
    if (shell->history_current->next) {
        shell->history_current = shell->history_current->next;
        return shell->history_current->command;
    }
    
    return ""; // Пустая строка для новой команды
}
