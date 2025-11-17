#ifndef BUILTINS_H
#define BUILTINS_H

int builtin_cd(char **argv);
int builtin_pwd(char **argv);
int builtin_echo(char **argv);
int builtin_exit(char **argv);
int builtin_help(char **argv);

int builtin_jobs(char **argv);
int builtin_fg(char **argv);
int builtin_bg(char **argv);
int builtin_kill(char **argv);

int is_builtin_command(char *command);
int handle_builtin(char **argv);

#endif
