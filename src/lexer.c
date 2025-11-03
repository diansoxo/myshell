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
//////////////////

char *handle_word(lexer_t *lexer) {
    int start_pos = lexer->position;
    int end_pos = lexer->position;
    
    // Определяем конец слова с учетом экранирования
    while (end_pos < lexer->length) {
        char current = lexer->input[end_pos];
        
        // Если встретили пробел или специальный символ без экранирования - конец слова
        if (is_whitespace(current) || is_special_char(current)) {
            break;
        }
        
        // Обработка экранирования
        if (current == '\\') {
            end_pos++; // Пропускаем обратный слеш
            if (end_pos < lexer->length) {
                end_pos++; // Учитываем экранированный символ
            }
        } else {
            end_pos++;
        }
    }
    
    int actual_len = 0;
    
    // Подсчитываем фактическую длину (без символов экранирования)
    int temp_pos = start_pos;
    while (temp_pos < end_pos) {
        if (lexer->input[temp_pos] == '\\') {
            temp_pos++; // Пропускаем обратный слеш
            if (temp_pos < end_pos) {
                actual_len++;
                temp_pos++;
            }
        } else {
            actual_len++;
            temp_pos++;
        }
    }
    
    if (actual_len == 0) {
        lexer->position = end_pos;
        return NULL;
    }
    
    char *result = (char*)malloc(actual_len + 1);
    if (result == NULL) {
        lexer->position = end_pos;
        return NULL;
    }
    
    // Копируем с обработкой экранирования
    int src_pos = start_pos;
    int dst_pos = 0;
    while (src_pos < end_pos && dst_pos < actual_len) {
        if (lexer->input[src_pos] == '\\') {
            src_pos++; // Пропускаем обратный слеш
            if (src_pos < end_pos) {
                result[dst_pos++] = lexer->input[src_pos++];
            }
        } else {
            result[dst_pos++] = lexer->input[src_pos++];
        }
    }
    result[dst_pos] = '\0';
    
    lexer->position = end_pos;
    return result;
}
//////////


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
