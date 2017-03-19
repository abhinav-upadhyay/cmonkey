/*-
 * Copyright (c) 2017 Abhinav Upadhyay <er.abhinav.upadhyay@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "token.h"

int
main(int argc, char **argv)
{
	const char *input = "let five = 5; \n"\
						 "let ten = 10;"\
						 "\n"\
						 "\n"\
						 "let add = fn(x, y) {\n"\
						 "	x + y;\n"\
						 "};\n"\
						 "\n"\
						 "let result = add(five, ten);";
	printf("%s\n", input);

	token tests[] = {
		{ LET, "let"},
		{ IDENT, "five"},
		{ ASSIGN, "="},
		{ INT, "5"},
		{ SEMICOLON, ";"},
		{ LET, "let"},
		{ IDENT, "ten"},
		{ ASSIGN, "="},
		{ INT, "10" },
		{ SEMICOLON, ";" },
		{ LET, "let" },
		{ IDENT, "add" },
		{ ASSIGN, "=" },
		{ FUNCTION, "fn" },
		{ LPAREN, "(" },
		{ IDENT, "x" },
		{ COMMA, "," },
		{ IDENT, "y" },
		{ RPAREN, ")" },
		{ LBRACE, "{" },
		{ IDENT, "x" },
		{ PLUS, "+" },
		{ IDENT, "y" },
		{ SEMICOLON, ";" },
		{ RBRACE, "}" },
		{ SEMICOLON, ";" },
		{ LET, "let" },
		{ IDENT, "result" },
		{ ASSIGN, "=" },
		{ IDENT, "add" },
		{ LPAREN, "(" },
		{ IDENT, "five" },
		{ COMMA, "," },
		{ IDENT, "ten" },
		{ RPAREN, ")" },
		{ SEMICOLON, ";" },
		{ END_OF_FILE, "" }

	};

	lexer *l = lexer_init((char *) input);
	int i = 0;
	token *t;
	for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
		t = lexer_next_token(l);
		assert (t->type == tests[i].type);
		assert(strcmp(t->literal, tests[i].literal) == 0);
		printf("Test %d passed\n", i);
	}
	return 0;
}
