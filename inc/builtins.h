#ifndef BUILTINS_H
#define BUILTINS_H

// Встроенные команды shell
int builtin_cd(char **argv);
int builtin_pwd(char **argv);
int builtin_echo(char **argv);
int builtin_exit(char **argv);
int builtin_help(char **argv);
int builtin_jobs(char **argv);//добавила
int builtin_fg(char **argv);
int builtin_bg(char **argv);
int builtin_kill(char **argv);

// Функции для работы с встроенными командами
int is_builtin_command(char *command);
int handle_builtin(char **argv);

#endif
