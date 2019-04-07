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

#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"

static const char * PROMPT = ">> ";

static void
print_parse_errors(parser_t *parser)
{
	cm_list_node *list_node = parser->errors->head;
	while (list_node) {
		printf("%s\n", (char *) list_node->data);
		list_node = list_node->next;
	}
}

int
main(int argc, char **argv)
{
	ssize_t bytes_read;
	size_t linesize = 0;
	char *line = NULL;
	lexer_t *l;
	parser_t *parser = NULL;
	program_t *program = NULL;
	token_t *tok;

	printf("%s", PROMPT);
	while ((bytes_read = getline(&line, &linesize, stdin)) != -1) {
		l = lexer_init(line);
		parser = parser_init(l);
		program = parse_program(parser);

		if (parser->errors) {
			print_parse_errors(parser);
			goto CONTINUE;
		}

		char *program_string = program->node.string(program);
		printf("%s\n", program_string);
		free(program_string);
CONTINUE:
		program_free(program);
		parser_free(parser);
		free(line);
		line = NULL;
		program = NULL;
		parser = NULL;
		printf("%s", PROMPT);
	}

	if (program)
		program_free(program);
	if (parser)
		parser_free(parser);
	if (line)
		free(line);
}
