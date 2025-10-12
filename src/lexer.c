#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "lexer.h"

lexer_t *lexer_create(const char *input) {
    lexer_t *lexer = (lexer_t*)malloc(sizeof(lexer_t));
    if (lexer == NULL) {
        return NULL;
    }
    
    lexer->input = input;
    lexer->position = 0;
    lexer->length = strlen(input);
    lexer->tokens = NULL;
    lexer->current_token = NULL;
    
    return lexer;
}
void lexer_destroy(lexer_t *lexer) {
    if (lexer == NULL) return;

    token_t *token = lexer->tokens;
    while (token != NULL) {
        token_t *next = token->next;
        if (token->value != NULL) {
            free(token->value);
        }
        free(token);
        token = next;
    }
    
    free(lexer);
}

static void add_token(lexer_t *lexer, token_type_t type, char *value) {
    token_t *new_token = (token_t*)malloc(sizeof(token_t));
    if (new_token == NULL) {
        return;
    }
    
    new_token->type = type;
    new_token->value = value;
    new_token->next = NULL;
    
    if (lexer->tokens == NULL) {
        lexer->tokens = new_token;
        lexer->current_token = new_token;
        return;
    }
    
    token_t *current = lexer->tokens;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_token;
}

int is_special_char(char c) {
    switch (c) {
        case '|':
        case '&':
        case ';':
        case '<':
        case '>':
        case '(':
        case ')':
            return 1;
        default:
            return 0;
    }
}

int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

char *handle_quotes(lexer_t *lexer, char quote_type) {
    int start_pos = lexer->position + 1;
    int len = 0;
    
    lexer->position++;
    
    while (lexer->position < lexer->length) {
        char current = lexer->input[lexer->position];
        
        if (current == quote_type) {
            break;
        }
        
        if (quote_type == '"' && current == '\\') {
            lexer->position++;
            if (lexer->position < lexer->length) {
                len++;
                lexer->position++;
            }
        } else {
            len++;
            lexer->position++;
        }
    }
    
    if (lexer->position >= lexer->length || lexer->input[lexer->position] != quote_type) {
        fprintf(stderr, "Ошибка: Незакрытая кавычка\n");
        return NULL;
    }
    
    char *result = (char*)malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }
    
    strncpy(result, lexer->input + start_pos, len);
    result[len] = '\0';
    
    lexer->position++;
    
    return result;
}

char *handle_word(lexer_t *lexer) {
    int len = 0;
    
    int temp_pos = lexer->position;
    while (temp_pos < lexer->length) {
        char current = lexer->input[temp_pos];
        
        if (is_whitespace(current) || is_special_char(current)) {
            break;
        }
        
        if (current == '\\') {
            temp_pos++;
            if (temp_pos < lexer->length) {
                len++;
                temp_pos++;
            }
        } else {
            len++;
            temp_pos++;
        }
    }
    
    if (len == 0) {
        return NULL;
    }
    
    char *result = (char*)malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }
    
    int i = 0;
    while (lexer->position < lexer->length && i < len) {
        char current = lexer->input[lexer->position];
        
        if (is_whitespace(current) || is_special_char(current)) {
            break;
        }
        
        if (current == '\\') {
            lexer->position++;
            if (lexer->position < lexer->length) {
                result[i++] = lexer->input[lexer->position];
                lexer->position++;
            }
        } else {
            result[i++] = current;
            lexer->position++;
        }
    }
    
    result[i] = '\0';
    return result;
}


token_t *lexer_tokenize(lexer_t *lexer) {
    if (lexer == NULL) {
        return NULL;
    }
    
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
            if (value != NULL) {
                add_token(lexer, TOKEN_WORD, value);
            }
            continue;
        }
        //спец символы
        if (current == '|' && lexer->position + 1 < lexer->length && 
            lexer->input[lexer->position + 1] == '|') {
            add_token(lexer, TOKEN_OR, strdup("||"));
            lexer->position += 2;
            continue;
        }
        
        if (current == '|' && lexer->position + 1 < lexer->length && 
            lexer->input[lexer->position + 1] == '&') {
            add_token(lexer, TOKEN_REDIR_ERR, strdup("|&"));
            lexer->position += 2;
            continue;
        }
        
        if (current == '&' && lexer->position + 1 < lexer->length && 
            lexer->input[lexer->position + 1] == '&') {
            add_token(lexer, TOKEN_AND, strdup("&&"));
            lexer->position += 2;
            continue;
        }
        
        if (current == '&' && lexer->position + 1 < lexer->length && 
            lexer->input[lexer->position + 1] == '>') {
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
        
        if (current == '>' && lexer->position + 1 < lexer->length && 
            lexer->input[lexer->position + 1] == '>') {
            add_token(lexer, TOKEN_REDIR_APPEND, strdup(">>"));
            lexer->position += 2;
            continue;
        }
        
        switch (current) {
            case '|':
                add_token(lexer, TOKEN_PIPE, strdup("|"));
                lexer->position++;
                continue;
                
            case '&':
                add_token(lexer, TOKEN_BACKGROUND, strdup("&"));
                lexer->position++;
                continue;
                
            case ';':
                add_token(lexer, TOKEN_SEMICOLON, strdup(";"));
                lexer->position++;
                continue;
                
            case '>':
                add_token(lexer, TOKEN_REDIR_OUT, strdup(">"));
                lexer->position++;
                continue;
                
            case '<':
                add_token(lexer, TOKEN_REDIR_IN, strdup("<"));
                lexer->position++;
                continue;
                
            case '(':
                add_token(lexer, TOKEN_LPAREN, strdup("("));
                lexer->position++;
                continue;
                
            case ')':
                add_token(lexer, TOKEN_RPAREN, strdup(")"));
                lexer->position++;
                continue;
        }
        
        char *word_value = handle_word(lexer);
        if (word_value != NULL) {
            add_token(lexer, TOKEN_WORD, word_value);
        } else {
            lexer->position++;
        }
    }
    
    add_token(lexer, TOKEN_EOF, NULL);
    
    lexer->current_token = lexer->tokens;
    
    return lexer->tokens;
}

token_t *lexer_get_next_token(lexer_t *lexer) {
    if (lexer == NULL || lexer->current_token == NULL) {
        return NULL;
    }
    
    token_t *token = lexer->current_token;
    lexer->current_token = token->next;
    return token;
}

void lexer_rewind(lexer_t *lexer) {
    if (lexer != NULL) {
        lexer->current_token = lexer->tokens;
    }
}
