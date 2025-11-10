#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>
#include "shell.h"
#include "lexer.h"
#include "parser.h"
#include "executor.h"

#define MAX_INPUT_LENGTH 1024

static char *get_username() { //получение имени пользователя
    struct passwd *pw = getpwuid(getuid());
    return pw ? strdup(pw->pw_name) : strdup("unknown");
}

// Получение имени хоста
static char *get_hostname() {
    static struct utsname uts;
    if (uname(&uts) == 0) {
        return strdup(uts.nodename);
    }
    return strdup("unknown");
}

// Получение текущей директории
static char *get_current_dir() {
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        return strdup("unknown");
    }
    
    // Укорачиваем домашнюю директорию до ~
    char *home = getenv("HOME");
    if (home != NULL && strncmp(cwd, home, strlen(home)) == 0) {
        char *short_path = malloc(strlen(cwd) - strlen(home) + 2);
        if (short_path) {
            short_path[0] = '~';
            strcpy(short_path + 1, cwd + strlen(home));
            free(cwd);
            return short_path;
        }
    }
    
    return cwd;
}

// Создание shell
shell_t *shell_create() {
    shell_t *shell = (shell_t*)malloc(sizeof(shell_t));
    if (shell == NULL) {
        return NULL;
    }
    
    shell->running = 1;
    shell->prompt = NULL;
    
    return shell;
}

// Уничтожение shell
void shell_destroy(shell_t *shell) {
    if (shell != NULL) {
        if (shell->prompt != NULL) {
            free(shell->prompt);
        }
        free(shell);
    }
}

// Печать приглашения
static void print_prompt() {
    char *username = get_username();
    char *hostname = get_hostname();
    char *current_dir = get_current_dir();
    
    printf("%s@%s:%s$ ", username, hostname, current_dir);
    fflush(stdout);
    
    free(username);
    free(hostname);
    free(current_dir);
}

// Чтение ввода пользователя
static char *read_input() {
    static char input[MAX_INPUT_LENGTH];
    
    if (fgets(input, MAX_INPUT_LENGTH, stdin) == NULL) {
        return NULL; // EOF (Ctrl+D)
    }
    
    // Убираем символ новой строки
    input[strcspn(input, "\n")] = '\0';
    
    return input;
}

// Обработка одной команды
static int process_command(const char *input) {
    if (strlen(input) == 0) {
        return 0; // Пустая команда
    }
    
    // Создаем лексер и парсим строку
    lexer_t *lexer = lexer_create(input);
    if (lexer == NULL) {
        fprintf(stderr, "Ошибка: не удалось создать лексер\n");
        return -1;
    }
    
    token_t *tokens = lexer_tokenize(lexer);
    if (tokens == NULL) {
        fprintf(stderr, "Ошибка: не удалось разобрать команду на токены\n");
        lexer_destroy(lexer);
        return -1;
    }
    
    // Создаем парсер и строим AST
    parser_t *parser = parser_create(lexer);
    if (parser == NULL) {
        fprintf(stderr, "Ошибка: не удалось создать парсер\n");
        lexer_destroy(lexer);
        return -1;
    }
    
    ast_node_t *ast = parse(parser);
    if (ast == NULL) {
        fprintf(stderr, "Ошибка: не удалось разобрать команду\n");
    } else {
        // Выполняем AST
        execute_ast(ast);
        ast_destroy(ast);
    }
    
    parser_destroy(parser);
    lexer_destroy(lexer);
    
    return 0;
}

// Основной цикл shell
void shell_run(shell_t *shell) {
    printf("Добро пожаловать в Simple Shell!\n");
    printf("Введите 'help' для списка команд, 'exit' для выхода\n\n");
    
    while (shell->running) {
        print_prompt();
        
        char *input = read_input();
        if (input == NULL) {
            printf("\nВыход из shell\n");
            break;
        }
        
        // Проверка на команду выхода
        if (strcmp(input, "exit") == 0) {
            break;
        }
        
        process_command(input);
    }
}