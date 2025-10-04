#ifndef TOKENS_H
#define TOKENS_H

typedef enum {
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_REDIR_APPEND,
    TOKEN_REDIR_ERR,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_SEMICOLON,
    TOKEN_BACKGROUND,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_EOF
} token_type_t;

typedef struct token {
    token_type_t type;
    char *value;
    struct token *next;
} token_t;

#endif
