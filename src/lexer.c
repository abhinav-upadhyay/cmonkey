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

#include <ctype.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "token.h"

lexer_t *
lexer_init(const char *input)
{
	lexer_t *l = malloc(sizeof(*l));
	if (l == NULL)
		err(EXIT_FAILURE, "malloc failed");

	l->input = strdup(input);
	if (l->input == NULL)
		err(EXIT_FAILURE, "strdup failed");

	l->current_offset = 0;
	l->read_offset = 1;
	l->ch = input[0];
	return l;
}

#define is_character(c) isalnum(c) || c == '_'

static char *
read_identifier(lexer_t *l)
{
	size_t position = l->current_offset;
	while (is_character(l->input[l->current_offset])) {
		l->current_offset++;
	}
	size_t nchars = l->current_offset - position;
	char *identifier = malloc(nchars + 1);
	memcpy(identifier, l->input + position, nchars); 
	identifier[nchars] = 0;
	l->read_offset = l->current_offset + 1;
	l->ch = l->input[l->current_offset];
	return identifier;
}

static void
read_char(lexer_t *l)
{
	if (l->ch) {
		l->current_offset = l->read_offset;
		l->read_offset++;
		l->ch = l->input[l->current_offset];
	}
}

static void
eat_whitespace(lexer_t *l)
{
	while (l->ch && (l->ch == ' ' || l->ch == '\n' || l->ch == '\r' || l->ch == '\t'))
		read_char(l);
}

token_t *
lexer_next_token(lexer_t *l)
{
	token_t *t;
	t = malloc(sizeof(*t));
	if (t == NULL)
		err(EXIT_FAILURE, "malloc failed");

	eat_whitespace(l);

	switch (l->ch) {
	case '=':	
		if (l->input[l->read_offset] == '=') {
			t->literal = strdup("==");
			t->type = EQ;
			read_char(l);
			read_char(l);
			break;
		}
		t->literal = strdup("=");
		t->type = ASSIGN;
		read_char(l);
		break;
	case '+':
		t->literal = strdup("+");
		t->type = PLUS;
		read_char(l);
		break;
	case ',':
		t->literal = strdup(",");
		t->type = COMMA;
		read_char(l);
		break;
	case ';':
		t->literal = strdup(";");
		t->type = SEMICOLON;
		read_char(l);
		break;
	case '(':
		t->literal = strdup("(");
		t->type = LPAREN;
		read_char(l);
		break;
	case ')':
		t->literal = strdup(")");
		t->type = RPAREN;
		read_char(l);
		break;
	case '{':
		t->literal = strdup("{");
		t->type = LBRACE;
		read_char(l);
		break;
	case '}':
		t->literal = strdup("}");
		t->type = RBRACE;
		read_char(l);
		break;
	case '!':
		if (l->input[l->read_offset] == '=') {
			t->literal = strdup("!=");
			t->type = NOT_EQ;
			read_char(l);
			read_char(l);
			break;
		}
		t->literal = strdup("!");
		t->type = BANG;
		read_char(l);
		break;
	case '-':
		t->literal = strdup("-");
		t->type = MINUS;
		read_char(l);
		break;
	case '/':
		t->literal = strdup("/");
		t->type = SLASH;
		read_char(l);
		break;
	case '*':
		t->literal = strdup("*");
		t->type = ASTERISK;
		read_char(l);
		break;
	case '<':
		t->literal = strdup("<");
		t->type = LT;
		read_char(l);
		break;
	case '>':
		t->literal = strdup(">");
		t->type = GT;
		read_char(l);
		break;
	case 0:
		t->literal = "";
		t->type = END_OF_FILE;
		break;
	default:
		if (is_character(l->ch)) {
			t->literal = read_identifier(l);
			t->type = get_token_type(t->literal);
		} else {
			t->literal = NULL;
			t->type = ILLEGAL;
			read_char(l);
	}
		break;
	}

	return t;
}

void
lexer_free(lexer_t *l)
{
	free(l->input);
	free(l);
}
