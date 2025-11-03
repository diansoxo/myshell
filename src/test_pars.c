#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"

void test_parser(const char *test_name, const char *input) {
    printf("%s\n", test_name);
    printf("Ввод: '%s'\n", input);
    
    lexer_t *lexer = lexer_create(input);
    if (lexer == NULL) {
        printf("Ошибка: не создался лексер\n\n");
        return;
    }
    
    token_t *tokens = lexer_tokenize(lexer);
    if (tokens == NULL) {
        printf("Ошибка: не получились токены\n\n");
        lexer_destroy(lexer);
        return;
    }
    
    printf("Токены: ");
    token_t *current = tokens;
    while (current != NULL && current->type != TOKEN_EOF) {
        const char *type_str;
        switch (current->type) {
            case TOKEN_WORD: type_str = "WORD"; break;
            case TOKEN_PIPE: type_str = "PIPE"; break;
            case TOKEN_REDIR_IN: type_str = "IN"; break;
            case TOKEN_REDIR_OUT: type_str = "OUT"; break;
            case TOKEN_REDIR_APPEND: type_str = "APPEND"; break;
            case TOKEN_REDIR_ERR: type_str = "ERR"; break;
            case TOKEN_AND: type_str = "AND"; break;
            case TOKEN_OR: type_str = "OR"; break;
            case TOKEN_SEMICOLON: type_str = "SEMI"; break;
            case TOKEN_BACKGROUND: type_str = "BG"; break;
            case TOKEN_LPAREN: type_str = "LBR"; break;
            case TOKEN_RPAREN: type_str = "RBR"; break;
            default: type_str = "UNKNOWN"; break;
        }
        printf("%s:'%s' ", type_str, current->value ? current->value : "NULL");
        current = current->next;
    }
    printf("\n");
    
    parser_t *parser = parser_create(lexer);
    if (parser == NULL) {
        printf("Ошибка: не создался парсер\n\n");
        lexer_destroy(lexer);
        return;
    }
    
    ast_node_t *ast = parse(parser);
    if (ast == NULL) {
        printf("Ошибка парсинга\n\n");
    } else {
        printf("AST:\n");
        ast_print(ast, 0);
        printf("OK\n");
        ast_destroy(ast);
    }
    
    parser_destroy(parser);
    lexer_destroy(lexer);
    printf("\n");
}

int main() {
    printf("\nТестирование парсера\n\n");
    
    test_parser("Просто", "ls");
    test_parser("С флагами", "ls -la");
    test_parser("С путем", "/bin/ls -l");
    
    test_parser("Вывод в файл", "echo text > file.txt");
    test_parser("Добавить в файл", "echo text >> file.txt");
    test_parser("Ввод из файла", "wc < input.txt");
    
    test_parser("Пайп", "ls | wc");
    test_parser("Два пайпа", "cat file | grep text | wc -l");
    
    test_parser("И", "mkdir dir && cd dir");
    test_parser("ИЛИ", "cd /none || echo error");
    
    test_parser("Точка с запятой", "ls; pwd");
    test_parser("В фоне", "sleep 5 &");
    
    test_parser("Скобки", "(ls && pwd)");
    test_parser("Скобки с пайпом", "(ls | wc) && echo done");
    
    test_parser("Все вместе", "ls -l | grep test > out.txt && echo finish");
    
    test_parser("Кавычки", "echo 'hello'");
    test_parser("Двойные кавычки", "echo \"hello world\"");
    
    test_parser("Ошибка - нет скобки", "(ls");
    test_parser("Ошибка - нет команды", "ls |");
    test_parser("Ошибка - пусто", "");
    
    printf("Конец тестов\n\n");
    
    return 0;
}
