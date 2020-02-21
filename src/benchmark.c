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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "compiler.h"
#include "evaluator.h"
#include "lexer.h"
#include "parser.h"
#include "vm.h"

static const char *INPUT = "let fib = fn(x) {\n"
    "   if (x == 0) {\n"
    "       return 0;\n"
    "   } else {\n"
    "       if (x == 1) {\n"
    "           return 1;\n"
    "       } else {\n"
    "           fib(x - 1) + fib(x - 2);\n"
    "       }\n"
    "   }\n"
    "};"
    "fib(35);";

int
main(int argc, char **argv)
{
   if (argc != 2)
        errx(EXIT_FAILURE, "Expected 2 arguments, got %d", argc);
   char *engine = argv[1];
   monkey_object_t *result;
   lexer_t *lexer = lexer_init(INPUT);
   parser_t *parser = parser_init(lexer);
   program_t *program = parse_program(parser);
   compiler_t *compiler = NULL;
   bytecode_t *bytecode = NULL;
   vm_t *vm = NULL;
   clock_t start;
   clock_t end;

   if (strcmp(engine, "vm") == 0) {
       compiler = compiler_init();
       compiler_error_t compile_error = compile(compiler, (node_t *) program);
       if (compile_error.code != COMPILER_ERROR_NONE) {
           fprintf(stderr, "Failed to compile the program with error: %s\n", compile_error.msg);
           free(compile_error.msg);
           goto EXIT;
       }
       bytecode = get_bytecode(compiler);
       vm = vm_init(bytecode);
       start = clock();
       vm_error_t vm_err = vm_run(vm);
       if (vm_err.code != VM_ERROR_NONE) {
           fprintf(stderr, "Faield to execute the program with error: %s\n", vm_err.msg);
           free(vm_err.msg);
           goto EXIT;
       }
       end = clock();
       result = vm_last_popped_stack_elem(vm);
   } else {
       environment_t *env = create_env();
       start = clock();
       result = monkey_eval((node_t *) program, env);
       end = clock();
       env_free(env);
   }
   char *result_str = result->inspect(result);
   printf("engine=%s, result=%s, duration=%f seconds\n", engine, result_str, (float) (end - start) / CLOCKS_PER_SEC);
   free(result_str);
   free_monkey_object(result);
   EXIT:
   parser_free(parser);
   program_free(program);
   if (compiler != NULL)
       compiler_free(compiler);
   if (vm != NULL)
       vm_free(vm);
   if (bytecode != NULL)
        bytecode_free(bytecode);
}
