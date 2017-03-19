#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "token.h"

int
main(int argc, char **argv)
{
	const char *input = "=+(){},;";
	token tests[] = {
		{ ASSIGN, "="},
		{ PLUS, "+"},
		{ LPAREN, "("},
		{ RPAREN, ")"},
		{ LBRACE, "{"},
		{ RBRACE, "}"},
		{ COMMA, ","},
		{ SEMICOLON, ";"}
	};

	lexer *l = lexer_init((char *) input);
	int i = 0;
	token *t;
	while ((t = lexer_next_token(l)) != NULL) {
		if (t->type == END_OF_FILE)
			break;
		assert (t->type == tests[i].type);
		assert(strcmp(t->literal, tests[i].literal) == 0);
		printf("Test %d passed\n", i);
		i++;
	}
	return 0;
}
