/*-
 * Copyright (c) 2019 Abhinav Upadhyay <er.abhinav.upadhyay@gmail.com>
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ast.h"
#include "builtins.h"
#include "cmonkey_utils.h"
#include "compiler.h"
#include "environment.h"
#include "evaluator.h"
#include "token.h"
#include "lexer.h"
#include "object.h"
#include "parser.h"
#include "vm.h"

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

static void
free_lines(cm_array_list *lines)
{
	for (size_t i = 0; i < lines->length; i++) {
		free(lines->array[i]);
	}
	lines->length = 0;
}

static int
execute_file(const char *filename)
{
	ssize_t bytes_read;
	size_t linesize = 0;
	char *line = NULL;
	char *program_string;
	lexer_t *l;
	parser_t *parser = NULL;
	program_t *program = NULL;

	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		switch (errno) {
			case EINVAL:
			case ENOMEM:
			case EACCES:
			case EINTR:
			case ELOOP:
			case EMFILE:
			case ENAMETOOLONG:
			case ENOENT:
			case EPERM:
			case EBADF:
				err(EXIT_FAILURE, "Failed to open file %s", filename);
			default:
				err(EXIT_FAILURE, "Failed to open file %s", filename);
		}
	}

	environment_t *env = create_env();
	cm_array_list *lines = cm_array_list_init(4, free);
	while ((bytes_read = getline(&line, &linesize, file)) != -1) {
		cm_array_list_add(lines, line);
		line = NULL;
		linesize = 0;
	}
	program_string = cm_array_string_list_join(lines, "\n");
	l = lexer_init(program_string);
	parser = parser_init(l);
	program = parse_program(parser);
	free(program_string);

	if (parser->errors) {
		print_parse_errors(parser);
		goto EXIT;
	}

	compiler_t *compiler = compiler_init();
	compiler_error_t compile_err = compile(compiler, (node_t *) program);
	if (compile_err.code != COMPILER_ERROR_NONE) {
		printf("Compile error: %s\n", compile_err.msg);
		free(compile_err.msg);
		goto EXIT;
	}

	bytecode_t *bytecode = get_bytecode(compiler);
	vm_t *machine = vm_init(bytecode);
	vm_error_t vm_err =  vm_run(machine);
	if (vm_err.code != VM_ERROR_NONE) {
		printf("VM Error: %s\n", vm_err.msg);
		free(vm_err.msg);
		goto EXIT;
	}
	monkey_object_t *top = vm_last_popped_stack_elem(machine);
	if (top != NULL) {
		if (top->type != MONKEY_NULL) {
			char *s = top->inspect(top);
			printf("%s\n", s);
			free(s);
		}
	}
	vm_free(machine);
	compiler_free(compiler);
	bytecode_free(bytecode);
	env_free(env);

EXIT:
	cm_array_list_free(lines);
	program_free(program);
	parser_free(parser);
	fclose(file);
	if (line)
		free(line);
	return 0;
}

static void
copy_globals(monkey_object_t *dst[GLOBALS_SIZE], monkey_object_t *src[GLOBALS_SIZE])
{
	for (size_t i = 0; i < GLOBALS_SIZE; i++) {
		if (dst[i] != NULL)
			free_monkey_object(dst[i]);
		if (src[i] == NULL)
			return;
		dst[i] = copy_monkey_object(src[i]);
	}
}

static void *
_copy_monkey_object(void *obj)
{
	return copy_monkey_object((monkey_object_t *) obj);
}

static int
repl(void)
{
	ssize_t bytes_read;
	size_t linesize = 0;
	char *line = NULL;
	char *program_string;
	lexer_t *l;
	parser_t *parser = NULL;
	program_t *program = NULL;
	compiler_t *compiler = NULL;
	bytecode_t *bytecode = NULL;
	monkey_object_t *globals[GLOBALS_SIZE] = {NULL};
	cm_array_list *constants = cm_array_list_init(16, free_monkey_object);
	symbol_table_t *symbol_table = symbol_table_init();
	for (size_t i = 0; i < get_builtins_count(); i++) {
		char *builtin_name = (char *) get_builtins_name(i);
		if (builtin_name == NULL)
			break;
		symbol_define_builtin(symbol_table, i, builtin_name);
	}

	vm_t *machine = NULL;
	environment_t *env = create_env();
	printf("%s\n", MONKEY_FACE);
	printf("Welcome to the monkey programming language\n");
	printf("%s", PROMPT);
	cm_array_list *lines = cm_array_list_init(4, free);
	while ((bytes_read = getline(&line, &linesize, stdin)) != -1) {
		if (strcmp(line, "quit\n") == 0)
			break;

		if (line[bytes_read - 2] == '\\') {
			line[bytes_read - 2] = 0;
			cm_array_list_add(lines, line);
			line = NULL;
			linesize = 0;
			printf("%s", "    ");
			continue;
		} else {
			cm_array_list_add(lines, line);
			line = NULL;
			linesize = 0;
		}

		program_string = cm_array_string_list_join(lines, "\n");
		l = lexer_init(program_string);
		parser = parser_init(l);
		program = parse_program(parser);

		if (parser->errors) {
			print_parse_errors(parser);
			goto CONTINUE;
		}
		compiler = compiler_init_with_state(symbol_table, constants);
		compiler_error_t compile_err = compile(compiler, (node_t *) program);
		if (compile_err.code != COMPILER_ERROR_NONE) {
			printf("Compiler error: %s\n", compile_err.msg);
			free(compile_err.msg);
			goto CONTINUE;
		}

		bytecode = get_bytecode(compiler);
		machine = vm_init_with_state(bytecode, globals);
		vm_error_t vm_err = vm_run(machine);
		if (vm_err.code != VM_ERROR_NONE) {
			printf("VM error: %s\n", vm_err.msg);
			free(vm_err.msg);
			goto CONTINUE;
		}
		monkey_object_t *top = vm_last_popped_stack_elem(machine);
		if (top != NULL) {
			char *s = top->inspect(top);
			printf("%s\n", s);
			free(s);
			free_monkey_object(top);
		}

CONTINUE:
		program_free(program);
		parser_free(parser);
		free_lines(lines);
		free(program_string);
		if (bytecode)
			bytecode_free(bytecode);
		if (compiler) {
			free_symbol_table(symbol_table);
			cm_array_list_free(constants);
			symbol_table = symbol_table_copy(compiler->symbol_table);
			constants = cm_array_list_copy(compiler->constants_pool, _copy_monkey_object);
			compiler_free(compiler);
		}
		if (machine) {
			copy_globals(globals, machine->globals);
			vm_free(machine);
		}
		line = NULL;
		program = NULL;
		parser = NULL;
		bytecode = NULL;
		compiler = NULL;
		machine = NULL;
		printf("%s", PROMPT);
	}

	if (program)
		program_free(program);
	if (parser)
		parser_free(parser);
	if (line)
		free(line);
	if (compiler)
		compiler_free(compiler);
	if (bytecode)
		bytecode_free(bytecode);
	if (machine)
		vm_free(machine);
	cm_array_list_free(lines);
	env_free(env);
	free_symbol_table(symbol_table);
	cm_array_list_free(constants);
	for (size_t i = 0; i < GLOBALS_SIZE; i++) {
		if (globals[i] == NULL)
			break;
		free_monkey_object(globals[i]);
	}
	return 0;
}

int
main(int argc, char **argv)
{
	if (argc == 1)
		return repl();
	if (argc == 2)
		return execute_file(argv[1]);
	errx(EXIT_FAILURE, "Unsupported numberof arguments %d", argc);
}
