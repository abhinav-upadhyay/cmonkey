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

lexer *
lexer_init(char *input)
{
	lexer *l = malloc(sizeof(*l));
	if (l == NULL)
		err(EXIT_FAILURE, "malloc failed");

	l->input = input;
	l->current_offset = 0;
	l->read_offset = 1;
	l->ch = input[0];
	return l;
}

#define is_character(c) isalnum(c) || c == '_'

static char *
read_identifier(lexer *l)
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
read_char(lexer *l)
{
	if (l->ch) {
		l->current_offset = l->read_offset;
		l->read_offset++;
		l->ch = l->input[l->current_offset];
	}
}

static int
is_number(char *literal)
{
	while (1) {
		char c = *literal;
		if (!c)
			break;

		if (!isdigit(c))
			return 0;
		literal++;
	}
	return 1;
}

static token_type
get_token_type(char *literal)
{
	if (strcmp(literal, "let") == 0)
		return LET;

	if (strcmp(literal, "fn") == 0)
		return FUNCTION;

	if (is_number(literal))
		return INT;

	return IDENT;
}

token *
lexer_next_token(lexer *l)
{
	token *t = malloc(sizeof(*t));
	if (t == NULL)
		err(EXIT_FAILURE, "malloc failed");

	switch (l->ch) {
	case '=':	
		t->literal = "=";
		t->type = ASSIGN;
		read_char(l);
		break;
	case '+':
		t->literal = "+";
		t->type = PLUS;
		read_char(l);
		break;
	case ',':
		t->literal = ",";
		t->type = COMMA;
		read_char(l);
		break;
	case ';':
		t->literal = ";";
		t->type = SEMICOLON;
		read_char(l);
		break;
	case '(':
		t->literal = "(";
		t->type = LPAREN;
		read_char(l);
		break;
	case ')':
		t->literal = ")";
		t->type = RPAREN;
		read_char(l);
		break;
	case '{':
		t->literal = "{";
		t->type = LBRACE;
		read_char(l);
		break;
	case '}':
		t->literal = "}";
		t->type = RBRACE;
		read_char(l);
		break;
	case '!':
		t->literal = "!";
		t->type = BANG;
		read_char(l);
		break;
	case '-':
		t->literal = "-";
		t->type = MINUS;
		read_char(l);
		break;
	case '/':
		t->literal = "/";
		t->type = SLASH;
		read_char(l);
		break;
	case '*':
		t->literal = "*";
		t->type = ASTERISK;
		read_char(l);
		break;
	case '<':
		t->literal = "<";
		t->type = LT;
		read_char(l);
		break;
	case '>':
		t->literal = ">";
		t->type = GT;
		read_char(l);
		break;
	case 0:
		t->literal = "";
		t->type = END_OF_FILE;
		break;
	case ' ':
	case '\t':
	case '\n':
		read_char(l);
		return lexer_next_token(l);
	default:
		if (is_character(*(l->input))) {
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

