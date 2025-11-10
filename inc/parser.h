#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

typedef struct {
    lexer_t *lexer;
    token_t *current_token;
} parser_t;

parser_t *parser_create(lexer_t *lexer);
void parser_destroy(parser_t *parser);
ast_node_t *parse(parser_t *parser);
ast_node_t *parse_command(parser_t *parser);
ast_node_t *parse_pipeline(parser_t *parser);
ast_node_t *parse_simple_command(parser_t *parser);

#endif

