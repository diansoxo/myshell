#include <stdio.h>
#include "lexer.h"

int main() {
    printf("Testing lexer...\n\n");
    
    char *test ="ls\\>";
    
    lexer_t *lexer = lexer_create(test);

    token_t *tokens = lexer_tokenize(lexer);
    
    token_t *current = tokens;
    while (current) {
        printf("Token: %d, Value: %s\n", 
               current->type, 
               current->value ? current->value : "NULL");
        current = current->next;
    }
    
    lexer_destroy(lexer);
    
    printf("\ntest completed!\n");
    return 0;
}
