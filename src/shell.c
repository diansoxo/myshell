#include <stdio.h>///изм
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

// ========== ПОЛУЧЕНИЕ ИНФОРМАЦИИ ДЛЯ ПРОМПТА ==========

// Получаем имя пользователя из системы
static char *get_username() {
    struct passwd *pw = getpwuid(getuid());  // Получаем информацию о пользователе
    return pw ? strdup(pw->pw_name) : strdup("unknown");  // Копируем имя или "unknown"
}

// Получаем имя компьютера (хоста)
static char *get_hostname() {
    static struct utsname uts;  // Структура для информации о системе
    
    if (uname(&uts) == 0) {
        return strdup(uts.nodename);  // Возвращаем имя узла
    }
    return strdup("unknown");  // Если ошибка - возвращаем "unknown"
}

// Получаем текущую директорию (сокращаем домашнюю до ~)
static char *get_current_dir() {
    // Получаем текущую рабочую директорию
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        return strdup("unknown");  // Если ошибка - возвращаем "unknown"
    }
    
    // Получаем путь к домашней директории
    char *home = getenv("HOME");
    
    // Проверяем находимся ли мы в домашней директории или ее поддиректориях
    if (home != NULL && strncmp(cwd, home, strlen(home)) == 0) {
        // Сокращаем путь: /home/user/documents -> ~/documents
        char *short_path = malloc(strlen(cwd) - strlen(home) + 2);
        if (short_path) {
            short_path[0] = '~';  // Начинаем с тильды
            strcpy(short_path + 1, cwd + strlen(home));  // Копируем остаток пути
            free(cwd);  // Освобождаем старый путь
            return short_path;
        }
    }
    
    // Если не в домашней директории - возвращаем полный путь
    return cwd;
}

// ========== ОСНОВНЫЕ ФУНКЦИИ SHELL ==========

// Создаем структуру shell - выделяем память и инициализируем
shell_t *shell_create(void) {
    shell_t *shell = (shell_t*)malloc(sizeof(shell_t));
    if (shell == NULL) {
        return NULL;  // Если не удалось выделить память
    }
    
    shell->running = 1;    // Shell активен и работает
    shell->prompt = NULL;  // Пока не используем кастомный prompt
    
    return shell;
}

// Освобождаем память когда shell закрывается
void shell_destroy(shell_t *shell) {
    if (shell != NULL) {
        if (shell->prompt != NULL) {
            free(shell->prompt);  // Освобождаем кастомный prompt если есть
        }
        free(shell);  // Освобождаем саму структуру
    }
}

// Печатаем приглашение командной строки: user@computer:directory$
static void print_prompt() {
    // Получаем информацию для prompt
    char *username = get_username();
    char *hostname = get_hostname();
    char *current_dir = get_current_dir();
    
    // Печатаем prompt в формате: user@computer:directory$
    printf("%s@%s:%s$ ", username, hostname, current_dir);
    fflush(stdout);  // Заставляем напечатать сразу (без буферизации)
    
    // Освобождаем память
    free(username);
    free(hostname);
    free(current_dir);
}

// Читаем что пользователь ввел с клавиатуры
static char *read_input() {
    static char input[MAX_INPUT_LENGTH];  // Буфер для ввода
    
    // Читаем строку из стандартного ввода
    if (fgets(input, MAX_INPUT_LENGTH, stdin) == NULL) {
        return NULL;  // Это случится если нажать Ctrl+D (конец файла)
    }
    
    // Убираем символ новой строки в конце (\n)
    input[strcspn(input, "\n")] = '\0';
    
    return input;
}

// Обрабатываем команду: разбираем и выполняем
static int process_command(const char *input) {
    // Проверяем пустая ли команда
    if (strlen(input) == 0) {
        return 0;  // Пустая команда - ничего не делаем
    }
    
    // ШАГ 1: Разбиваем строку на токены (слова)
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
    
    // ШАГ 2: Строим дерево команд из токенов
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
        // ШАГ 3: Выполняем команду
        execute_ast(ast);
        ast_destroy(ast);  // Очищаем память дерева
    }
    
    // Очищаем все что создали
    parser_destroy(parser);
    lexer_destroy(lexer);
    
    return 0;
}

// Главный цикл shell - работает пока пользователь не выйдет
void shell_run(shell_t *shell) {
    printf("Введите 'help' для списка команд, 'exit' для выхода\n\n");

    // Настраиваем обработку сигналов (Ctrl+C, завершение процессов и т.д.)
    setup_signal_handlers();
    
    // Бесконечный цикл чтения команд
    while (shell->running) {
        // Печатаем приглашение: user@computer:directory$
        print_prompt();
        
        // Читаем что ввел пользователь
        char *input = read_input();
        
        // Проверяем не нажал ли пользователь Ctrl+D
        if (input == NULL) {
            printf("\nВыход из shell\n");
            break;  // Выход по Ctrl+D
        }
        
        // Проверяем специальные команды
        if (strcmp(input, "exit") == 0) {
            break;  // Выход по команде exit
        }
        
        // Обрабатываем обычную команду
        process_command(input);
    }
}
