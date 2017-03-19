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

#include <err.h>
#include <stdlib.h>

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
		break;
	case '+':
		t->literal = "+";
		t->type = PLUS;
		break;
	case ',':
		t->literal = ",";
		t->type = COMMA;
		break;
	case ';':
		t->literal = ";";
		t->type = SEMICOLON;
		break;
	case '(':
		t->literal = "(";
		t->type = LPAREN;
		break;
	case ')':
		t->literal = ")";
		t->type = RPAREN;
		break;
	case '{':
		t->literal = "{";
		t->type = LBRACE;
		break;
	case '}':
		t->literal = "}";
		t->type = RBRACE;
		break;
	case 0:
		t->literal = "";
		t->type = END_OF_FILE;
		break;
	default:
		t->literal = NULL;
		t->type = ILLEGAL;
		break;
	}

	if (l->ch) {
		l->current_offset = l->read_offset;
		l->read_offset++;
		l->ch = l->input[l->current_offset];
	}
	return t;
}

