#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "parser.h"

parser_t *parser_create(lexer_t *lexer) {
    if (lexer == NULL) {
        return NULL;
    }
    
    parser_t *parser = (parser_t*)malloc(sizeof(parser_t));
    if (parser == NULL) {
        return NULL;
    }
    
    parser->lexer = lexer;
    parser->current_token = lexer->tokens;
    
    return parser;
}

void parser_destroy(parser_t *parser) {
    if (parser != NULL) {
        free(parser);
    }
}

static token_t *parser_peek(parser_t *parser) {
    return parser->current_token;
}

static token_t *parser_consume(parser_t *parser, token_type_t expected_type) {
    if (parser->current_token == NULL || parser->current_token->type != expected_type) {
        return NULL;
    }
    
    token_t *token = parser->current_token;
    parser->current_token = parser->current_token->next;
    return token;
}

static ast_node_t *parse_redirects(parser_t *parser, ast_node_t *command_node);

ast_node_t *parse(parser_t *parser) {
    if (parser == NULL) {
        return NULL;
    }
    
    return parse_pipeline(parser);
}

ast_node_t *parse_pipeline(parser_t *parser) {
    ast_node_t *left = parse_command(parser);
    if (left == NULL) {
        return NULL;
    }
    
    while (parser_peek(parser) != NULL) {
        token_t *token = parser_peek(parser);
        
        if (token->type == TOKEN_PIPE) {
            parser_consume(parser, TOKEN_PIPE);
            
            ast_node_t *right = parse_command(parser);
            if (right == NULL) {
                ast_destroy(left);
                fprintf(stderr, "Ошибка: ожидается команда после '|'\n");
                return NULL;
            }
            
            ast_node_t *pipe_node = ast_create_node(NODE_PIPE);
            if (pipe_node == NULL) {
                ast_destroy(left);
                ast_destroy(right);
                return NULL;
            }
            
            pipe_node->left = left;
            pipe_node->right = right;
            left = pipe_node;
            
        } else if (token->type == TOKEN_REDIR_ERR && strcmp(token->value, "|&") == 0) {
            parser_consume(parser, TOKEN_REDIR_ERR);
            
            ast_node_t *right = parse_command(parser);
            if (right == NULL) {
                ast_destroy(left);
                fprintf(stderr, "Ошибка: ожидается команда после '|&'\n");
                return NULL;
            }
            
            ast_node_t *pipe_node = ast_create_node(NODE_PIPE);
            if (pipe_node == NULL) {
                ast_destroy(left);
                ast_destroy(right);
                return NULL;
            }
            
            pipe_node->left = left;
            pipe_node->right = right;
            pipe_node->redirect_err = 1;
            left = pipe_node;
            
        } else {
            break;
        }
    }
    
    return left;
}

ast_node_t *parse_command(parser_t *parser) {
    ast_node_t *node = parse_simple_command(parser);
    if (node == NULL) {
        return NULL;
    }
    
    node = parse_redirects(parser, node);
    if (node == NULL) {
        return NULL;
    }
    
    while (parser_peek(parser) != NULL) {
        token_t *token = parser_peek(parser);
        ast_node_t *new_node = NULL;
        node_type_t node_type;
        
        switch (token->type) {
            case TOKEN_AND:
                parser_consume(parser, TOKEN_AND);
                node_type = NODE_AND;
                break;
                
            case TOKEN_OR:
                parser_consume(parser, TOKEN_OR);
                node_type = NODE_OR;
                break;
                
            case TOKEN_SEMICOLON:
                parser_consume(parser, TOKEN_SEMICOLON);
                node_type = NODE_SEMICOLON;
                break;
                
            case TOKEN_BACKGROUND:
                parser_consume(parser, TOKEN_BACKGROUND);
                node_type = NODE_BACKGROUND;
                new_node = ast_create_node(node_type);
                if (new_node == NULL) {
                    ast_destroy(node);
                    return NULL;
                }
                new_node->left = node;
                new_node->right = NULL;
                node = new_node;
                continue;
                
            default:
                return node;
        }
        
        new_node = ast_create_node(node_type);
        if (new_node == NULL) {
            ast_destroy(node);
            return NULL;
        }
        
        new_node->left = node;
        new_node->right = parse_command(parser);
        if (new_node->right == NULL) {
            ast_destroy(new_node);
            fprintf(stderr, "Ошибка: ожидается команда после оператора\n");
            return node;
        }
        
        node = new_node;
    }
    
    return node;
}

ast_node_t *parse_simple_command(parser_t *parser) {
    if (parser_peek(parser) != NULL && parser_peek(parser)->type == TOKEN_LPAREN) {
        parser_consume(parser, TOKEN_LPAREN);
        
        ast_node_t *subshell_node = ast_create_node(NODE_SUBSHELL);
        if (subshell_node == NULL) {
            return NULL;
        }
        
        subshell_node->left = parse_pipeline(parser);
        if (subshell_node->left == NULL) {
            ast_destroy(subshell_node);
            fprintf(stderr, "Ошибка: ожидается команда внутри скобок\n");
            return NULL;
        }
        
        if (parser_peek(parser) == NULL || parser_peek(parser)->type != TOKEN_RPAREN) {
            ast_destroy(subshell_node);
            fprintf(stderr, "Ошибка: ожидается закрывающая скобка ')'\n");
            return NULL;
        }
        parser_consume(parser, TOKEN_RPAREN);
        
        return subshell_node;
    }
    
    char **argv = NULL;
    int argc = 0;
    int capacity = 10;
    
    argv = (char**)malloc(capacity * sizeof(char*));
    if (argv == NULL) {
        return NULL;
    }
    
    while (parser_peek(parser) != NULL && parser_peek(parser)->type == TOKEN_WORD) {
        token_t *word_token = parser_consume(parser, TOKEN_WORD);
        
        if (argc >= capacity) {
            capacity *= 2;
            char **new_argv = (char**)realloc(argv, capacity * sizeof(char*));
            if (new_argv == NULL) {
                for (int i = 0; i < argc; i++) {
                    free(argv[i]);
                }
                free(argv);
                return NULL;
            }
            argv = new_argv;
        }
        
        argv[argc] = strdup(word_token->value);
        if (argv[argc] == NULL) {
            for (int i = 0; i < argc; i++) {
                free(argv[i]);
            }
            free(argv);
            return NULL;
        }
        argc++;
    }
    
    if (argc == 0) {
        free(argv);
        return NULL;
    }
    
    ast_node_t *command_node = ast_create_command_node(argv, argc);
    if (command_node == NULL) {
        for (int i = 0; i < argc; i++) {
            free(argv[i]);
        }
        free(argv);
        return NULL;
    }
    
    return command_node;
}

static ast_node_t *parse_redirects(parser_t *parser, ast_node_t *command_node) {
    while (parser_peek(parser) != NULL) {
        token_t *token = parser_peek(parser);
        
        switch (token->type) {
            case TOKEN_REDIR_IN: {
                parser_consume(parser, TOKEN_REDIR_IN);
                token_t *file_token = parser_consume(parser, TOKEN_WORD);
                if (file_token == NULL) {
                    ast_destroy(command_node);
                    fprintf(stderr, "Ошибка: ожидается имя файла после '<'\n");
                    return NULL;
                }
                
                if (command_node->in_file != NULL) {
                    free(command_node->in_file);
                }
                command_node->in_file = strdup(file_token->value);
                if (command_node->in_file == NULL) {
                    ast_destroy(command_node);
                    return NULL;
                }
                break;
            }
                
            case TOKEN_REDIR_OUT: {
                parser_consume(parser, TOKEN_REDIR_OUT);
                token_t *file_token = parser_consume(parser, TOKEN_WORD);
                if (file_token == NULL) {
                    ast_destroy(command_node);
                    fprintf(stderr, "Ошибка: ожидается имя файла после '>'\n");
                    return NULL;
                }
                
                if (command_node->out_file != NULL) {
                    free(command_node->out_file);
                }
                command_node->out_file = strdup(file_token->value);
                if (command_node->out_file == NULL) {
                    ast_destroy(command_node);
                    return NULL;
                }
                command_node->append = 0;
                break;
            }
                
            case TOKEN_REDIR_APPEND: {
                parser_consume(parser, TOKEN_REDIR_APPEND);
                token_t *file_token = parser_consume(parser, TOKEN_WORD);
                if (file_token == NULL) {
                    ast_destroy(command_node);
                    fprintf(stderr, "Ошибка: ожидается имя файла после '>>'\n");
                    return NULL;
                }
                
                if (command_node->out_file != NULL) {
                    free(command_node->out_file);
                }
                command_node->out_file = strdup(file_token->value);
                if (command_node->out_file == NULL) {
                    ast_destroy(command_node);
                    return NULL;
                }
                command_node->append = 1;
                break;
            }
                
            case TOKEN_REDIR_ERR: {
                const char *redirect_op = token->value;
                parser_consume(parser, TOKEN_REDIR_ERR);
                token_t *file_token = parser_consume(parser, TOKEN_WORD);
                if (file_token == NULL) {
                    ast_destroy(command_node);
                    fprintf(stderr, "Ошибка: ожидается имя файла после '%s'\n", redirect_op);
                    return NULL;
                }
                
                if (command_node->err_file != NULL) {
                    free(command_node->err_file);
                }
                command_node->err_file = strdup(file_token->value);
                if (command_node->err_file == NULL) {
                    ast_destroy(command_node);
                    return NULL;
                }
                
                if (strcmp(redirect_op, "&>>") == 0) {
                    command_node->append = 1;
                } else {
                    command_node->append = 0;
                }
                break;
            }
                
            default:
                return command_node;
        }
    }
    
    return command_node;
}
