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
#include <string.h>

#include "ast.h"
#include "evaluator.h"
#include "object.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"

static const char * PROMPT = ">> ";
static const char *MONKEY_FACE = "            __,__\n\
   .--.  .-\"     \"-.  .--.\n\
  / .. \\/  .-. .-.  \\/ .. \\\n\
 | |  '|  /   Y   \\  |'  | |\n\
 | \\   \\  \\ 0 | 0 /  /   / |\n\
  \\ '- ,\\.-\"\"\"\"\"\"\"-./, -' /\n\
   ''-' /_   ^ ^   _\\ '-''\n\
       |  \\._   _./  |\n\
       \\   \\ '~' /   /\n\
        '._ '-=-' _.'\n\
           '-----'\n\
";


static void
print_parse_errors(parser_t *parser)
{
	printf("%s\n", MONKEY_FACE);
	printf("Woops! We ran into some monkey business here!\n");
	printf(" Parser errors:\n");
	cm_list_node *list_node = parser->errors->head;
	while (list_node) {
		printf("\t%s\n", (char *) list_node->data);
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
	printf("%s\n", MONKEY_FACE);
	printf("Welcome to the monkey programming language\n");
	printf("%s", PROMPT);
	while ((bytes_read = getline(&line, &linesize, stdin)) != -1) {
		if (strcmp(line, "quit\n") == 0)
			goto QUIT;
		l = lexer_init(line);
		parser = parser_init(l);
		program = parse_program(parser);

		if (parser->errors) {
			print_parse_errors(parser);
			goto CONTINUE;
		}

		monkey_object_t *evaluated = monkey_eval((node_t *) program);
		if (evaluated != NULL) {
			char *s = evaluated->inspect(evaluated);
			printf("%s\n", s);
			free(s);
			free_monkey_object(evaluated);
		}

CONTINUE:
		program_free(program);
		parser_free(parser);
		free(line);
		line = NULL;
		program = NULL;
		parser = NULL;
		printf("%s", PROMPT);
	}
QUIT:

	if (program)
		program_free(program);
	if (parser)
		parser_free(parser);
	if (line)
		free(line);
}
