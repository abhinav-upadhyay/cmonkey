#ifndef TOKEN_H
#define TOKEN_H

typedef enum token_type {
	ILLEGAL,
	END_OF_FILE,

	// identifiers, literals
	IDENT,
	INT,

	//operators
	ASSIGN,
	PLUS,

	//delimiters
	COMMA,
	SEMICOLON,
	LPAREN,
	RPAREN,
	LBRACE,
	RBRACE,

	//keywords
	FUNCTION,
	LET
} token_type;

typedef struct {
	token_type type;
	char *literal;
} token;

#endif




