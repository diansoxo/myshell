#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "lexer.h"


lexer_t *lexer_create(const char *input) {
    lexer_t *lexer = malloc(sizeof(lexer_t));
    if (!lexer) return NULL;
    
    lexer->input = input;
    lexer->position = 0;
    lexer->length = strlen(input);
    lexer->tokens = NULL;
    lexer->current_token = NULL;
    
    return lexer;
}

void lexer_destroy(lexer_t *lexer) {
    if (!lexer) return;
    
    token_t *current = lexer->tokens;
    while (current) {
        token_t *next = current->next;
        if (current->value) free(current->value);
        free(current);
        current = next;
    }
    free(lexer);
}

static void add_token(lexer_t *lexer, token_type_t type, char *value) {
    token_t *token = malloc(sizeof(token_t));
    if (!token) return;
    
    token->type = type;
    token->value = value;
    token->next = NULL;
    
    if (!lexer->tokens) {
        lexer->tokens = token;
    } else {
        token_t *current = lexer->tokens;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = token;
    }
    
    if (!lexer->current_token) {
        lexer->current_token = lexer->tokens;
    }
}

int is_special_char(char c) {
    return strchr("|&;<>()", c) != NULL;
}

int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

char *handle_quotes(lexer_t *lexer, char quote_type) {
    int start = lexer->position + 1;
    int length = 0;
    
    lexer->position++;
    
    while (lexer->position < lexer->length && 
           lexer->input[lexer->position] != quote_type) {
        if (quote_type == '"' && lexer->input[lexer->position] == '\\') {
            lexer->position++;
            if (lexer->position < lexer->length) {
                length++;
                lexer->position++;
            }
        } else {
            length++;
            lexer->position++;
        }
    }
    
    if (lexer->position >= lexer->length) {
        fprintf(stderr, "Ошибка: Незакрытая кавычка\n");
        return NULL;
    }
    
    char *result = malloc(length + 1);
    if (!result) return NULL;
    
    strncpy(result, lexer->input + start, length);
    result[length] = '\0';
    
    lexer->position++;
    return result;
}

char *handle_word(lexer_t *lexer) {
    int start = lexer->position;
    int length = 0;
    
    while (lexer->position < lexer->length) {
        char c = lexer->input[lexer->position];
        
        if (is_whitespace(c) || is_special_char(c)) {
            break;
        }
        
        if (c == '\\') {
            lexer->position++;
            if (lexer->position < lexer->length) {
                length++;
                lexer->position++;
            }
        } else {
            length++;
            lexer->position++;
        }
    }
    
    if (length == 0) return NULL;
    
    char *result = malloc(length + 1);
    if (!result) return NULL;
    
    strncpy(result, lexer->input + start, length);
    result[length] = '\0';
    
    return result;
}

token_t *lexer_tokenize(lexer_t *lexer) {
    if (!lexer) return NULL;
    
    lexer->position = 0;
    
    while (lexer->position < lexer->length) {
        char current = lexer->input[lexer->position];
        
        if (is_whitespace(current)) {
            lexer->position++;
            continue;
        }
        
        if (current == '#') {
            break;
        }
        
        if (current == '\'' || current == '"') {
            char *value = handle_quotes(lexer, current);
            if (value) {
                add_token(lexer, TOKEN_WORD, value);
            }
            continue;
        }
        
        if (current == '|' && lexer->position + 1 < lexer->length) {
            if (lexer->input[lexer->position + 1] == '|') {
                add_token(lexer, TOKEN_OR, strdup("||"));
                lexer->position += 2;
                continue;
            }
        }
        
        if (current == '|' && lexer->position + 1 < lexer->length) {
            if (lexer->input[lexer->position + 1] == '&') {
                add_token(lexer, TOKEN_REDIR_ERR, strdup("|&"));
                lexer->position += 2;
                continue;
            }
        }
        if (current == '&' && lexer->position + 1 < lexer->length) {
            if (lexer->input[lexer->position + 1] == '&') {
                add_token(lexer, TOKEN_AND, strdup("&&"));
                lexer->position += 2;
                continue;
            }
        }
        
        if (current == '&' && lexer->position + 1 < lexer->length) {
            if (lexer->input[lexer->position + 1] == '>') {
                if (lexer->position + 2 < lexer->length && 
                    lexer->input[lexer->position + 2] == '>') {
                    add_token(lexer, TOKEN_REDIR_ERR, strdup("&>>"));
                    lexer->position += 3;
                } else {
                    add_token(lexer, TOKEN_REDIR_ERR, strdup("&>"));
                    lexer->position += 2;
                }
                continue;
            }
        }
        
        if (current == '>' && lexer->position + 1 < lexer->length) {
            if (lexer->input[lexer->position + 1] == '>') {
                add_token(lexer, TOKEN_REDIR_APPEND, strdup(">>"));
                lexer->position += 2;
                continue;
            }
        }
        
        if (current == '|') {
            add_token(lexer, TOKEN_PIPE, strdup("|"));
            lexer->position++;
            continue;
        }
        
        if (current == '&') {
            add_token(lexer, TOKEN_BACKGROUND, strdup("&"));
            lexer->position++;
            continue;
        }
        
        if (current == ';') {
            add_token(lexer, TOKEN_SEMICOLON, strdup(";"));
            lexer->position++;
            continue;
        }
        
        if (current == '>') {
            add_token(lexer, TOKEN_REDIR_OUT, strdup(">"));
            lexer->position++;
            continue;
        }
        
        if (current == '<') {
            add_token(lexer, TOKEN_REDIR_IN, strdup("<"));
            lexer->position++;
            continue;
        }
        
        if (current == '(') {
            add_token(lexer, TOKEN_LPAREN, strdup("("));
            lexer->position++;
            continue;
        }
        
        if (current == ')') {
            add_token(lexer, TOKEN_RPAREN, strdup(")"));
            lexer->position++;
            continue;
        }
        
        char *value = handle_word(lexer);
        if (value) {
            add_token(lexer, TOKEN_WORD, value);
        } else {
            lexer->position++;
        }
    }
    
    add_token(lexer, TOKEN_EOF, NULL);
    
    lexer->current_token = lexer->tokens;
    
    return lexer->tokens;
}

token_t *lexer_get_next_token(lexer_t *lexer) {
    if (!lexer || !lexer->current_token) return NULL;
    
    token_t *current = lexer->current_token;
    lexer->current_token = current->next;
    return current;
}

void lexer_rewind(lexer_t *lexer) {
    if (lexer) {
        lexer->current_token = lexer->tokens;
    }
}
