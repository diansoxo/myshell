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

static char *get_username() {// Получаем имя пользователя из системы
    struct passwd *pw = getpwuid(getuid());  // Получаем информацию о пользователе
    return pw ? strdup(pw->pw_name) : strdup("unknown");
}


static char *get_hostname() {// Получаем имя компьютера
    static struct utsname uts;
    
    if (uname(&uts) == 0) {
        return strdup(uts.nodename);
    }
    return strdup("unknown");
}


static char *get_current_dir() {// Получаем текущую директорию (сокращаем домашнюю до ~)
    char *cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        return strdup("unknown");
    }
    
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


shell_t *shell_create(void) {// Создаем структуру shell - выделяем память и инициализируем
    shell_t *shell = (shell_t*)malloc(sizeof(shell_t));
    if (shell == NULL) {
        return NULL; 
    }
    
    shell->running = 1;
    shell->prompt = NULL;
    
    return shell;
}


void shell_destroy(shell_t *shell) {// Освобождаем память когда shell закрывается
    if (shell != NULL) {
        if (shell->prompt != NULL) {
            free(shell->prompt);
        }
        free(shell);
    }
}

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

static char *read_input() {//изм
    size_t capacity = INPUT_CHUNK_SIZE;
    size_t len = 0;
    char *buffer = malloc(capacity);//динамическое выделение памяти
    
    if (buffer == NULL) {
        return NULL;
    }
    
    buffer[0] = '\0';
    
    while (1) {
        if (fgets(buffer + len, capacity - len, stdin) == NULL) {// Читаем по кусочкам
            if (len == 0) {
                free(buffer);
                return NULL;  // Ctrl+D или EOF
            }
            break;
        }
        
        len += strlen(buffer + len);
        
        if (len > 0 && buffer[len - 1] == '\n') {// Проверяем, есть ли символ новой строки
            buffer[len - 1] = '\0';// Убираем \n
            
            char *trimmed = realloc(buffer, len);// Оптимизируем память - обрезаем буфер до нужного размера
            if (trimmed != NULL) {
                return trimmed;
            }
            return buffer;  // Если realloc не удался, возвращаем как есть
        }
        
        if (len + 1 >= capacity) {
            capacity *= 2;
            char *new_buffer = realloc(buffer, capacity);//увеличение буфера при необходимости
            if (new_buffer == NULL) {
                free(buffer);//Память освобождается
                return NULL;
            }
            buffer = new_buffer;
        }
    }
    
    return buffer;
}

static int process_command(const char *input) {// Обрабатываем команду: разбираем и выполняем
    if (strlen(input) == 0) { // Проверяем пустая ли команда
        return 0;  // Пустая команда - ничего не делаем
    }

    
    lexer_t *lexer = lexer_create(input);//Разбиваем строку на токены (слова)
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
    
    
    parser_t *parser = parser_create(lexer);// Строим дерево команд из токенов
    if (parser == NULL) {
        fprintf(stderr, "Ошибка: не удалось создать парсер\n");
        lexer_destroy(lexer);
        return -1;
    }
    
    ast_node_t *ast = parse(parser);
    if (ast == NULL) {
        fprintf(stderr, "Ошибка: не удалось разобрать команду\n");
    } else {
        
        execute_ast(ast);//Выполняем команду
        ast_destroy(ast);
    }
    
    parser_destroy(parser);
    lexer_destroy(lexer);
    
    return 0;
}


void shell_run(shell_t *shell) {// Главный цикл shell - работает пока пользователь не выйдет
    printf("Введите 'help' для списка команд, 'exit' для выхода\n\n");

    setup_signal_handlers();
    
    while (shell->running) {
        print_prompt();

        char *input = read_input();

        if (input == NULL) {
            printf("\nВыход из shell\n");
            break;  // Выход по Ctrl+D
        }
 
        if (strcmp(input, "exit") == 0) {
            break;  // Выход по команде exit
        }
        
        // Обрабатываем обычную команду
        process_command(input);
    }
}
