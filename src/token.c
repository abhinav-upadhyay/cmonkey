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
#include <stdlib.h>
#include <string.h>

#include "token.h"

void
token_free(token_t *tok)
{
	switch (tok->type) {
	case LET:	
	case FUNCTION:
	case IF:
	case ELSE:
	case RETURN:
	case TRUE:
	case FALSE:
	case INT:
	case IDENT:
		free(tok->literal);
		break;
	default:
		break;
	}

	free(tok);
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

token_type
get_token_type(char *literal)
{
	if (strcmp(literal, "let") == 0)
		return LET;

	if (strcmp(literal, "fn") == 0)
		return FUNCTION;

	if (strcmp(literal, "if") == 0)
		return IF;

	if (strcmp(literal, "else") == 0)
		return ELSE;

	if (strcmp(literal, "return") == 0)
		return RETURN;

	if (strcmp(literal, "true") == 0)
		return TRUE;

	if (strcmp(literal, "false") == 0)
		return FALSE;

	if (is_number(literal))
		return INT;

	return IDENT;
}

token_t *
token_copy(token_t *src)
{
	token_t *copy = malloc(sizeof(*copy));
	if (copy == NULL)
		return NULL;
	copy->type = src->type;
	copy->literal = strdup(src->literal);
	if (copy->literal == NULL) {
		free(copy);
		return NULL;
	}
	return copy;
}


