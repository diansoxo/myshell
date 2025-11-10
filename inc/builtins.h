#ifndef BUILTINS_H
#define BUILTINS_H

// Встроенные команды shell
int builtin_cd(char **args);
int builtin_pwd(char **args);
int builtin_echo(char **args);
int builtin_exit(char **args);
int builtin_help(char **args);

// Функции для работы с встроенными командами
int is_builtin_command(char *command);
int handle_builtin(char **args);

#endif