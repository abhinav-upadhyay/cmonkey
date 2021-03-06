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

#ifndef TOKEN_H
#define TOKEN_H

typedef enum token_type {
	ILLEGAL,
	END_OF_FILE,

	// identifiers, literals
	IDENT,
	INT,
	STRING,

	//operators
	ASSIGN,
	PLUS,
	MINUS,
	BANG,
	SLASH,
	ASTERISK,
	PERCENT,
	LT,
	GT,
	EQ,
	NOT_EQ,
	AND,
	OR,

	//delimiters
	COMMA,
	SEMICOLON,
	LPAREN,
	RPAREN,
	LBRACE,
	RBRACE,
	LBRACKET,
	RBRACKET,
	COLON,

	//keywords
	FUNCTION,
	LET,
	IF,
	ELSE,
	RETURN,
	TRUE,
	FALSE,
	WHILE
} token_type;

static const char *token_names[] = {
	"ILLEGAL",
	"END_OF_FILE",

	// identifiers, literals
	"IDENT",
	"INT",
	"STRING",

	//operators
	"ASSIGN",
	"PLUS",
	"MINUS",
	"BANG",
	"SLASH",
	"ASTERISK",
	"PERCENT",
	"LT",
	"GT",
	"EQ",
	"NOT_EQ",
	"AND",
	"OR",

	//delimiters
	"COMMA",
	"SEMICOLON",
	"LPAREN",
	"RPAREN",
	"LBRACE",
	"RBRACE",
	"LBRACKET",
	"RBRACKET",
	"COLON",

	//keywords
	"FUNCTION",
	"LET",
	"IF",
	"ELSE",
	"RETURN",
	"TRUE",
	"FALSE",
	"WHILE"
};

#define get_token_name(tok) token_names[tok->type]
#define get_token_name_from_type(tok_type) token_names[tok_type]

typedef struct token_t {
	token_type type;
	char *literal;
} token_t;

void token_free(token_t *);
token_t *token_copy(token_t *);
token_type get_token_type(char *);
#endif
