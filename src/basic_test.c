#include <stdio.h>
#include <assert.h>
#include "../inc/lexer.h"
#include "../inc/parser.h"

void test_lexer() {
    printf("Тестирование лексера...\n");
    
    // Тест простой команды
    lexer_t *lexer = lexer_create("ls -l /home");
    token_t *tokens = lexer_tokenize(lexer);
    
    assert(tokens != NULL);
    assert(tokens->type == TOKEN_WORD);
    assert(strcmp(tokens->value, "ls") == 0);
    
    printf("Тест лексера пройден!\n");
    lexer_destroy(lexer);
}

void test_parser() {
    printf("Тестирование парсера...\n");
    
    lexer_t *lexer = lexer_create("echo hello world");
    token_t *tokens = lexer_tokenize(lexer);
    parser_t *parser = parser_create(lexer);
    ast_node_t *ast = parse(parser);
    
    assert(ast != NULL);
    assert(ast->type == NODE_COMMAND);
    assert(ast->argc == 3);
    
    printf("Тест парсера пройден!\n");
    
    ast_destroy(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
}

int main() {
    test_lexer();
    test_parser();
    printf("Все тесты пройдены успешно!\n");
    return 0;
}