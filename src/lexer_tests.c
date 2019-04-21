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
#include "test_utils.h"

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
			 "let result = add(five, ten);\n"\
			 "!-/*5;\n"\
			 "5 < 10 > 5;\n"\
			 "if (5 < 10 ) {\n"\
			 "	return true;\n"\
			 "} else {\n"\
			 "	return false;\n"\
			 "}\n"\
			 "\n"\
			 "10 == 10;\n"\
			 "10 != 9;\n"\
			 "!5;\n"\
			 "return 10; 10;\n"\
			 "\"foobar\"\n"\
			 "\"foo bar\"\n"\
			 "[1, 2];";

	token_t tests[] = {
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
		{ BANG, "!" },
		{ MINUS, "-" },
		{ SLASH, "/" },
		{ ASTERISK, "*" },
		{ INT, "5" },
		{ SEMICOLON, ";"},
		{ INT, "5" },
		{ LT, "<" },
		{ INT, "10" },
		{ GT, ">" },
		{ INT, "5" },
		{ SEMICOLON, ";" },
		{ IF, "if" },
		{ LPAREN, "(" },
		{ INT, "5" },
		{ LT, "<" },
		{ INT, "10" },
		{ RPAREN, ")" },
		{ LBRACE, "{" },
		{ RETURN, "return" },
		{ TRUE, "true" },
		{ SEMICOLON, ";" },
		{ RBRACE, "}" },
		{ ELSE, "else" },
		{ LBRACE, "{" },
		{ RETURN, "return" },
		{ FALSE, "false" },
		{ SEMICOLON, ";" },
		{ RBRACE, "}" },
		{ INT, "10" },
		{ EQ, "==" },
		{ INT, "10" },
		{ SEMICOLON, ";" },
		{ INT, "10" },
		{ NOT_EQ, "!=" },
		{ INT, "9" },
		{ SEMICOLON, ";" },
		{BANG, "!"},
		{INT, "5"},
		{SEMICOLON, ";"},
		{RETURN, "return"},
		{INT, "10"},
		{SEMICOLON, ";"},
		{INT, "10"},
		{SEMICOLON, ";"},
		{STRING, "foobar"},
		{STRING, "foo bar"},
		{LBRACKET, "["},
		{INT, "1"},
		{COMMA, ","},
		{INT, "2"},
		{RBRACKET, "]"},
		{SEMICOLON, ";"},
		{ END_OF_FILE, "" }
	};

	lexer_t *l = lexer_init((char *) input);
	int i = 0;
	token_t *t;
	for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
		t = lexer_next_token(l);
		printf("Testing lexing for input %s\n", tests[i].literal);
		test(t->type == tests[i].type, "Expected token %s, got %s\n",
			get_token_name_from_type(tests[i].type), get_token_name_from_type(t->type));
		test(strcmp(t->literal, tests[i].literal) == 0,
			"Expected literal %s, found %s\n", tests[i].literal, t->literal);
		token_free(t);
	}
	lexer_free(l);
	return 0;
}
