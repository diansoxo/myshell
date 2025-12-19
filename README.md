# myshell
собственный интерпретатор командной строки (shell)

ОТЧЕТ

АРХИТЕКТУРА: Лексер -> Парсер -> AST -> Исполнитель

КОМПОНЕНТЫ:

1. REPL (shell.c) - главный цикл
   Приглашение: user@host:~/path$
   Динамическое чтение ввода с расширением буфера

2. Лексер lexer.c- 22 типа токенов
   Операторы: && || ; & | |&
   Перенаправления: > >> < &> &>>
   Кавычки: одинарные/двойные с экранированием

3. Парсер parser.c, ast.c- 8 типов узлов AST
   NODE_COMMAND: простая команда с аргументами
   NODE_PIPE: конвейер между командами
   NODE_REDIRECT: перенаправление ввода-вывода
   NODE_AND: условное && (выполнить если успешно)
   NODE_OR: условное || (выполнить если неудачно)
   NODE_SEMICOLON: последовательное выполнение ;
   NODE_BACKGROUND: фоновое выполнение &
   NODE_SUBSHELL: команда в скобках

4. Исполнитель executor.c - fork/exec/pipe
   execute_simple_command: встроенная или внешняя команда через fork/exec
   execute_pipeline: pipe() + dup2() для передачи данных между процессами
   execute_and/or: условное выполнение по exit code
   execute_redirect: dup2() для перенаправления файловых дескрипторов
   launch_process: fork создаёт процесс, execvp запускает программу

5. Встроенные команды builtins.c- 9 команд
   cd: смена директории (chdir)
   pwd: текущая директория (getcwd)
   echo: вывод текста с флагом -n
   exit: выход с кодом
   help: справка по командам
   jobs: список фоновых задач
   fg: перевод задачи на передний план
   bg: продолжить задачу в фоне
   kill: завершить задачу

6. Управление задачами job_control.c
   Связный список job_t с полями: job_id, pgid, command, state
   Функции: create_job, add_job, remove_job, find_job
   Обработчик SIGCHLD для отслеживания завершения процессов

СТРУКТУРА ФАЙЛОВ

src
  main.c- точка входа (shell_create -> shell_run -> shell_destroy)
  shell.c- REPL цикл и приглашение
  lexer.c- токенизация строки
  parser.c- построение AST
  ast.c- работа с узлами дерева
  executor.c- выполнение команд
  builtins.c- встроенные команды
  job_control.c- фоновые задачи
  test_parser.c- тесты

inc
  shell.h, lexer.h, parser.h, ast.h, executor.h, builtins.h, job_control.h, tokens.h

Makefile:
  make- компиляция в bin/main
  make run- запуск shell
  make valrun- проверка памяти через valgrind
  make debug- отладка через gdb
  make clean- очистка

Примеры команд:
  ls -la | grep shell (конвейер)
  ls > out.txt (перенаправление)
  mkdir test && cd test && pwd(условное &&)
  cd /bad || echo "Failed"(условное ||)
  ls ; pwd ; echo done(последовательность ;)
  sleep 1000 & (фоновое выполнение)
  jobs(список фоновых задач)
  fg 1 (на передний план)

Реализовано:
  REPL с приглашением user@host:~/path$
  Лексер: 22 токена, обработка кавычек
  Парсер: AST с 8 типами узлов
  Исполнитель: fork/exec, pipe, dup2

  9 встроенных команд
  Перенаправления: > >> < &> &>>
  Конвейеры: | |&
  Операторы: && || ; &
  Job control: jobs fg bg kill
  Обработка ошибок: лексические, синтаксические, выполнения
  по valgrind 0 утечек
