#include <stdlib.h>///изменила
#include <string.h>
#include <stdio.h>
#include "parser.h"

static ast_node_t *parse_redirects(parser_t *parser, ast_node_t *command_node);


parser_t *parser_create(lexer_t *lexer) {
    if (lexer == NULL) {
        return NULL;
    }
    
    
    parser_t *parser = (parser_t*)malloc(sizeof(parser_t));// Выделяем память для парсера
    if (parser == NULL) {
        return NULL;
    }
    
    
    parser->lexer = lexer;// Сохраняем лексер и начинаем с первого токена
    parser->current_token = lexer->tokens;
    
    return parser;
}


void parser_destroy(parser_t *parser) {// Освобождаем память парсера
    if (parser != NULL) {
        free(parser);
    }
}


static token_t *parser_peek(parser_t *parser) {// Смотрим на текущий токен, но не переходим к следующему
    return parser->current_token;
}


static token_t *parser_consume(parser_t *parser, token_type_t expected_type) {// Берем текущий токен и переходим к следующему
    
    if (parser->current_token == NULL || parser->current_token->type != expected_type) {// Проверяем что токен правильного типа
        return NULL;
    }
    
   
    token_t *token = parser->current_token; // Сохраняем токен и двигаемся вперед
    parser->current_token = parser->current_token->next;
    return token;
}


ast_node_t *parse(parser_t *parser) {
    if (parser == NULL) {
        return NULL;
    }
    
   
    return parse_pipeline(parser);// Начинаем с разбора конвейеров
}


ast_node_t *parse_pipeline(parser_t *parser) {// Разбираем конвейеры: команда1 | команда2 | команда3
    
    ast_node_t *left = parse_command(parser);//Переменная left теперь содержит узел первой команды (до конвейера)
    if (left == NULL) {
        return NULL;
    }
    
    
    while (parser_peek(parser) != NULL) {// Пока есть символы |, добавляем их в конвейер
        token_t *token = parser_peek(parser);
        
        
        if (token->type == TOKEN_PIPE) {// Обычный конвейер |
            parser_consume(parser, TOKEN_PIPE);
            
            
            ast_node_t *right = parse_command(parser);// Разбираем правую команду
            if (right == NULL) {
                ast_destroy(left);
                fprintf(stderr, "Ошибка: ожидается команда после '|'\n");
                return NULL;
            }
            
            
            ast_node_t *pipe_node = ast_create_node(NODE_PIPE);// Создаем узел конвейера
            if (pipe_node == NULL) {
                ast_destroy(left);
                ast_destroy(right);
                return NULL;
            }
            
            pipe_node->left = left;
            pipe_node->right = right;
            left = pipe_node;
            
        } 
       
        else if (token->type == TOKEN_REDIR_ERR && strcmp(token->value, "|&") == 0) { // Конвейер с перенаправлением ошибок |&
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
            pipe_node->data.pipe.redirect_err = 1; // Помечаем что это |&
            left = pipe_node;
            
        } else {
            break; //Больше нет конвейеров
        }
    }
    
    return left;
}


ast_node_t *parse_command(parser_t *parser) {// Разбираем команды с операторами: &&, ||, ;, &
    
    ast_node_t *node = parse_simple_command(parser);// Сначала разбираем простую команду
    if (node == NULL) {
        return NULL;
    }
    
    
    node = parse_redirects(parser, node);// Добавляем перенаправления если есть
    if (node == NULL) {
        return NULL;
    }
    
    
    while (parser_peek(parser) != NULL) {// Обрабатываем операторы: cmd1 && cmd2, cmd1 || cmd2, cmd1 ; cmd2
        token_t *token = parser_peek(parser);
        ast_node_t *new_node = NULL;
        node_type_t node_type;
        
        
        switch (token->type) {//Определяем тип оператора
            case TOKEN_AND:
                parser_consume(parser, TOKEN_AND);
                node_type = NODE_AND;// &&
                break;
                
            case TOKEN_OR:
                parser_consume(parser, TOKEN_OR);
                node_type = NODE_OR; // ||
                break;
                
            case TOKEN_SEMICOLON:
                parser_consume(parser, TOKEN_SEMICOLON);
                node_type = NODE_SEMICOLON; // ;
                break;
                
            case TOKEN_BACKGROUND:
                parser_consume(parser, TOKEN_BACKGROUND);
                node_type = NODE_BACKGROUND; //&
                
                new_node = ast_create_node(node_type);// Создаем узел для &
                if (new_node == NULL) {
                    ast_destroy(node);
                    return NULL;
                }
                new_node->left = node;
                new_node->right = NULL; // У & нет правой части
                node = new_node;
                continue;// Переходим к следующему токену
                
            default:
                return node;// Больше нет операторов
        }
        
        
        new_node = ast_create_node(node_type);// Создаем узел для оператора (кроме &)
        if (new_node == NULL) {
            ast_destroy(node);
            return NULL;
        }
        
        new_node->left = node;
        new_node->right = parse_command(parser);// Разбираем правую команду
        
        if (new_node->right == NULL) {
            ast_destroy(new_node);
            fprintf(stderr, "Ошибка: ожидается команда после оператора\n");
            return node;
        }
        
        node = new_node;
    }
    
    return node;
}


ast_node_t *parse_simple_command(parser_t *parser) {// Разбираем простую команду или команду в скобках
    
    if (parser_peek(parser) != NULL && parser_peek(parser)->type == TOKEN_LPAREN) {// Проверяем есть ли открывающая скобка(подсекция)
        parser_consume(parser, TOKEN_LPAREN);
        
        ast_node_t *subshell_node = ast_create_node(NODE_SUBSHELL);//Создаем узел для подсекции
        if (subshell_node == NULL) {
            return NULL;
        }
        
        subshell_node->left = parse_pipeline(parser);// Разбираем команды внутри скобок
        if (subshell_node->left == NULL) {
            ast_destroy(subshell_node);
            fprintf(stderr, "Ошибка: ожидается команда внутри скобок\n");
            return NULL;
        }
        
        if (parser_peek(parser) == NULL || parser_peek(parser)->type != TOKEN_RPAREN) {// Проверяем закрывающую скобку
            ast_destroy(subshell_node);
            fprintf(stderr, "Ошибка: ожидается закрывающая скобка ')'\n");
            return NULL;
        }
        parser_consume(parser, TOKEN_RPAREN);
        
        return subshell_node;
    }
    

    char **argv = NULL; //собираем все слова
    int argc = 0;
    int capacity = 10;  // Начальный размер массива
    
    argv = (char**)malloc(capacity * sizeof(char*));
    if (argv == NULL) {
        return NULL;
    }
    
    memset(argv, 0, capacity * sizeof(char*));
    
    
    while (parser_peek(parser) != NULL && parser_peek(parser)->type == TOKEN_WORD) {// Собираем все слова команды (ls -l /home)
        token_t *word_token = parser_consume(parser, TOKEN_WORD);
        
        
        if (argc >= capacity) {// Если массив заполнен, увеличиваем его
            capacity *= 2;
            char **new_argv = (char**)realloc(argv, capacity * sizeof(char*));
            if (new_argv == NULL) {
                
                for (int i = 0; i < argc; i++) {// Очищаем уже выделенную память
                    free(argv[i]);
                }
                free(argv);
                return NULL;
            }
            argv = new_argv;
            
            memset(argv + argc, 0, (capacity - argc) * sizeof(char*));//заполняем новую память NULLами
        }
        
        
        argv[argc] = strdup(word_token->value);// Копируем слово в массив
        if (argv[argc] == NULL) {
            for (int i = 0; i < argc; i++) {
                free(argv[i]);
            }
            free(argv);
            return NULL;
        }
        argc++;
    }
    
    
    if (argc == 0) {// Если не нашли ни одного слова - ошибка
        free(argv);
        return NULL;
    }
    
    if (argc < capacity) {
        argv[argc] = NULL;
    }
    
    
    ast_node_t *command_node = ast_create_command_node(argv, argc);// Создаем узел для собранных аргументов
    if (command_node == NULL) {//если не удалось создать узел, очищаем память
        for (int i = 0; i < argc; i++) {
            free(argv[i]);//осв кажд строку
        }
        free(argv);
        return NULL;
    }
    
    return command_node;
}


static ast_node_t *parse_redirects(parser_t *parser, ast_node_t *command_node) {//перенаправления например cmd > out.txt измм
    while (parser_peek(parser) != NULL) {
        token_t *token = parser_peek(parser);
        
        if (token->type == TOKEN_REDIR_IN) {
            parser_consume(parser, TOKEN_REDIR_IN);
            token_t *file_token = parser_consume(parser, TOKEN_WORD);
            
            if (file_token == NULL) {
                ast_destroy(command_node);
                fprintf(stderr, "Ошибка: ожидается имя файла после '<'\n");
                return NULL;
            }
            
            
            ast_node_t *redirect_node = ast_create_node(NODE_REDIRECT);// изм создали узел перенаправления
            if (redirect_node == NULL) {
                ast_destroy(command_node);
                return NULL;
            }
            redirect_node->left = command_node;
            redirect_node->data.redirect.in_file = strdup(file_token->value);//изм
            command_node = redirect_node;
        }
        else if (token->type == TOKEN_REDIR_OUT) {
            parser_consume(parser, TOKEN_REDIR_OUT);
            token_t *file_token = parser_consume(parser, TOKEN_WORD);
            
            if (file_token == NULL) {
                ast_destroy(command_node);
                fprintf(stderr, "Ошибка: ожидается имя файла после '>'\n");
                return NULL;
            }
            
            ast_node_t *redirect_node = ast_create_node(NODE_REDIRECT);
            if (redirect_node == NULL) {
                ast_destroy(command_node);
                return NULL;
            }
            redirect_node->left = command_node;
            redirect_node->data.redirect.out_file = strdup(file_token->value);  //изм
            redirect_node->data.redirect.append = 0;  //изм
            command_node = redirect_node;
        }
        else if (token->type == TOKEN_REDIR_APPEND) {
            parser_consume(parser, TOKEN_REDIR_APPEND);
            token_t *file_token = parser_consume(parser, TOKEN_WORD);
            
            if (file_token == NULL) {
                ast_destroy(command_node);
                fprintf(stderr, "Ошибка: ожидается имя файла после '>>'\n");
                return NULL;
            }
            
            ast_node_t *redirect_node = ast_create_node(NODE_REDIRECT);
            if (redirect_node == NULL) {
                ast_destroy(command_node);
                return NULL;
            }
            redirect_node->left = command_node;
            redirect_node->data.redirect.out_file = strdup(file_token->value);  //изм
            redirect_node->data.redirect.append = 1;  //изм
            command_node = redirect_node;
        }
        else if (token->type == TOKEN_REDIR_ERR) {
            const char *redirect_op = token->value;
            parser_consume(parser, TOKEN_REDIR_ERR);
            token_t *file_token = parser_consume(parser, TOKEN_WORD);
            
            if (file_token == NULL) {
                ast_destroy(command_node);
                fprintf(stderr, "Ошибка: ожидается имя файла после '%s'\n", redirect_op);
                return NULL;
            }
            
            ast_node_t *redirect_node = ast_create_node(NODE_REDIRECT);
            if (redirect_node == NULL) {
                ast_destroy(command_node);
                return NULL;
            }
            redirect_node->left = command_node;
            redirect_node->data.redirect.err_file = strdup(file_token->value);  //изм
            
            
            if (strcmp(redirect_op, "&>>") == 0) {// Определяем режим: перезапись или добавление
                redirect_node->data.redirect.append = 1;//изм
            } else {
                redirect_node->data.redirect.append = 0;//изм
            }
            command_node = redirect_node;
        }
        else {
            break; //больше нет перенаправлений
        }
    }
    
    return command_node;
}
