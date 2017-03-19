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

