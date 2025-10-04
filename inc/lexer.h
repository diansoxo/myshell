#ifndef LEXER_H
#define LEXER_H

#include "tokens.h"

typedef struct {
    const char *input;
    int position;
    int length;
    token_t *tokens;
    token_t *current_token;
} lexer_t;

lexer_t *lexer_create(const char *input);
void lexer_destroy(lexer_t *lexer);
token_t *lexer_tokenize(lexer_t *lexer);
token_t *lexer_get_next_token(lexer_t *lexer);
void lexer_rewind(lexer_t *lexer);

int is_special_char(char c);
int is_whitespace(char c);
char *handle_quotes(lexer_t *lexer, char quote_type);
char *handle_word(lexer_t *lexer);

#endif
