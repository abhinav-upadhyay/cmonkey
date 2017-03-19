#include <stdio.h>
#include <stdlib.h>

#include "token.h"
#include "lexer.h"

static const char * PROMPT = ">> ";

int
main(int argc, char **argv)
{
	ssize_t bytes_read;
	size_t linesize = 0;
	char *line = NULL;
	lexer *l;
	token *tok;

	printf("%s", PROMPT);
	while ((bytes_read = getline(&line, &linesize, stdin)) != -1) {
		l = lexer_init(line);
		for (tok = lexer_next_token(l);
			tok->type != END_OF_FILE;
			tok = lexer_next_token(l))
			printf("{Type: %s Literal: %s}\n", token_names[tok->type], tok->literal);
		free(line);
		line = NULL;
		printf("%s", PROMPT);
	}

}
