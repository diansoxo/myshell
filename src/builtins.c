#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "builtins.h"

// Смена директории
int builtin_cd(char **args) {
    if (args[1] == NULL) {
        // Если аргумента нет, переходим в домашнюю директорию
        char *home = getenv("HOME");
        if (home == NULL) {
            fprintf(stderr, "cd: HOME не установлена\n");
            return 1;
        }
        if (chdir(home) != 0) {
            perror("cd");
            return 1;
        }
    } else {
        if (chdir(args[1]) != 0) {
            perror("cd");
            return 1;
        }
    }
    return 0;
}

// Печать текущей директории
int builtin_pwd(char **args) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    } else {
        perror("pwd");
        return 1;
    }
}

// Команда echo
int builtin_echo(char **args) {
    int i = 1;
    int newline = 1;
    
    // Проверка флага -n
    if (args[1] != NULL && strcmp(args[1], "-n") == 0) {
        newline = 0;
        i = 2;
    }
    
    // Печать аргументов
    while (args[i] != NULL) {
        printf("%s", args[i]);
        if (args[i + 1] != NULL) {
            printf(" ");
        }
        i++;
    }
    
    if (newline) {
        printf("\n");
    }
    
    return 0;
}

// Выход из shell
int builtin_exit(char **args) {
    int exit_code = 0;
    if (args[1] != NULL) {
        exit_code = atoi(args[1]);
    }
    exit(exit_code);
}

// Справка
int builtin_help(char **args) {
    printf("Simple Shell - Встроенные команды:\n\n");
    printf("  cd [директория]- сменить текущую директорию\n");
    printf("  pwd- показать текущую директорию\n");
    printf("  echo [текст]- вывести текст\n");
    printf("  exit [код]- выйти из shell\n");
    printf("  help- показать эту справку\n\n");
    
    printf("Операторы:\n");
    printf("  cmd1 | cmd2- конвейер (передать вывод cmd1 в cmd2)\n");
    printf("  cmd1 && cmd2- выполнить cmd2 только если cmd1 успешна\n");
    printf("  cmd1 || cmd2- выполнить cmd2 только если cmd1 неуспешна\n");
    printf("  cmd1 ; cmd2- выполнить команды последовательно\n");
    printf("  cmd &- выполнить команду в фоне\n\n");
    
    printf("Перенаправления:\n");
    printf("  cmd > file- записать вывод в file\n");
    printf("  cmd >> file- добавить вывод в file\n");
    printf("  cmd < file- читать ввод из file\n");
    printf("  cmd &> file- перенаправить stdout и stderr в file\n");
    printf("  cmd &>> file- добавить stdout и stderr в file\n");
    
    return 0;
}

// Проверка является ли команда встроенной
int is_builtin_command(char *command) {
    if (command == NULL) return 0;
    
    char *builtins[] = {
        "cd", "pwd", "echo", "exit", "help", NULL
    };
    
    for (int i = 0; builtins[i] != NULL; i++) {
        if (strcmp(command, builtins[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Обработка встроенной команды
int handle_builtin(char **args) {
    if (args[0] == NULL) return 0;
    
    if (strcmp(args[0], "cd") == 0) {
        return builtin_cd(args);
    } else if (strcmp(args[0], "pwd") == 0) {
        return builtin_pwd(args);
    } else if (strcmp(args[0], "echo") == 0) {
        return builtin_echo(args);
    } else if (strcmp(args[0], "exit") == 0) {
        return builtin_exit(args);
    } else if (strcmp(args[0], "help") == 0) {
        return builtin_help(args);
    }
    
    return 0;
}