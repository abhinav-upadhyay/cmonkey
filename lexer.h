#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct {
	char *input;
	size_t current_offset;
	size_t read_offset;
	char ch;
} lexer;

lexer * lexer_init(char *);
token * lexer_next_token(lexer *);

#endif
