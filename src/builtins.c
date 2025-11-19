\#include <stdio.h>//измм
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "builtins.h"

//встроенные команды шелла


int builtin_cd(char **argv) {//Команда cd-сменить текущую директорию
    if (argv[1] == NULL) {
        char *home = getenv("HOME");// Если аргумента нет, переходим в домашнюю директорию
        if (home == NULL) {
            fprintf(stderr, "cd: HOME не установлена\n");
            return 1;
        }
        if (chdir(home) != 0) {
            perror("cd");
            return 1;
        }
    } else {
        if (chdir(argv[1]) != 0) { //Меняем директорию на указанную
            perror("cd");
            return 1;
        }
    }
    return 0; //Успешно
}


int builtin_pwd(char **argv) {// Команда pwd - показать текущую директорию
    (void)argv; // Игнорируем аргументы
    
    char cwd[1024];  // Буфер для текущей директории
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);  // Печатаем текущую директорию
        return 0;
    } else {
        perror("pwd");//Печатаем ошибку если не удалось
        return 1;
    }
}

int builtin_echo(char **argv) {// Команда echo - напечатать текст
    int i = 1;// Начинаем с первого аргумента (argv[0] это "echo")
    int newline = 1;//По умолчанию печатаем перевод строки в конце
    
    if (argv[1] != NULL && strcmp(argv[1], "-n") == 0) {// Проверяем флаг -n (не печатать перевод строки)
        newline = 0;  // Не печатать перевод строки
        i = 2; // Пропускаем флаг -n
    }
    
    
    while (argv[i] != NULL) {// Печатаем все аргументы через пробел
        printf("%s", argv[i]);
        if (argv[i + 1] != NULL) {
            printf(" "); // Печатаем пробел между аргументами
        }
        i++;
    }
    
    if (newline) {// Печатаем перевод строки если нужно
        printf("\n");
    }
    return 0;
}

int builtin_exit(char **argv) {// Команда exit - выйти из shell
    // Просто возвращаем код выхода, а shell сам завершится
    if (argv[1] != NULL) {
        return atoi(argv[1]);  // Код выхода из аргумента
    }
    return 0;  // успешный выход
}


int builtin_help(char **argv) {// Команда help - показать справку
    (void)argv; // Игнорируем аргументы
    
    printf("Simple Shell - Встроенные команды:\n\n");
    printf("  cd [директория] - сменить текущую директорию\n");
    printf("  pwd - показать текущую директорию\n");
    printf("  echo [текст] - вывести текст\n");
    printf("  exit [код] - выйти из shell\n");
    printf("  help - показать эту справку\n");
    printf("  jobs - показать фоновые задачи\n");
    printf("  fg <job_id> - перевести задачу в foreground\n");
    printf("  bg <job_id> - перевести задачу в background\n");
    printf("  kill <job_id> - завершить задачу\n\n");
    
    printf("Операторы:\n");
    printf("  cmd1 | cmd2 - конвейер (передать вывод cmd1 в cmd2)\n");
    printf("  cmd1 && cmd2 - выполнить cmd2 только если cmd1 успешна\n");
    printf("  cmd1 || cmd2 - выполнить cmd2 только если cmd1 неуспешна\n");
    printf("  cmd1 ; cmd2 - выполнить команды последовательно\n");
    printf("  cmd & - выполнить команду в фоне\n\n");
    
    printf("Перенаправления:\n");
    printf("  cmd > file - записать вывод в file\n");
    printf("  cmd >> file - добавить вывод в file\n");
    printf("  cmd < file - читать ввод из file\n");
    printf("  cmd &> file - перенаправить stdout и stderr в file\n");
    printf("  cmd &>> file - добавить stdout и stderr в file\n");
    
    return 0;
}

//функции для работы со встроенными командами


int is_builtin_command(char *command) {// Проверяем является ли команда встроенной
    if (command == NULL) return 0;  // Пустая команда - не встроенная
    
    
    char *builtins[] = {// Список всех встроенных команд нашего shell
        "cd", "pwd", "echo", "exit", "help", 
        "jobs", "fg", "bg", "kill", NULL
    };
    
    
    for (int i = 0; builtins[i] != NULL; i++) {// Проверяем есть ли команда в списке
        if (strcmp(command, builtins[i]) == 0) {
            return 1;  // Да, это встроенная команда
        }
    }
    return 0;// Нет, это внешняя команда
}


int handle_builtin(char **argv) {// Обрабатываем встроенную команду (вызываем нужную функцию)
    if (argv[0] == NULL) return 0;  // Пустая команда
    
    if (strcmp(argv[0], "cd") == 0) {// В зависимости от команды вызываем соответствующую функцию
        return builtin_cd(argv);
    } else if (strcmp(argv[0], "pwd") == 0) {
        return builtin_pwd(argv);
    } else if (strcmp(argv[0], "echo") == 0) {
        return builtin_echo(argv);
    } else if (strcmp(argv[0], "exit") == 0) {
        return builtin_exit(argv);
    } else if (strcmp(argv[0], "help") == 0) {
        return builtin_help(argv);
    } else if (strcmp(argv[0], "jobs") == 0) {
        return builtin_jobs(argv);
    } else if (strcmp(argv[0], "fg") == 0) {
        return builtin_fg(argv);
    } else if (strcmp(argv[0], "bg") == 0) {
        return builtin_bg(argv);
    } else if (strcmp(argv[0], "kill") == 0) {
        return builtin_kill(argv);
    }
    
    return 0;  // Неизвестная команда
}
