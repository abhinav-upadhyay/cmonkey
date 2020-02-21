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

#ifndef VM_H
#define VM_H

#include <stdlib.h>
#include "cmonkey_utils.h"
#include "compiler.h"
#include "frame.h"
#include "object.h"
#include "opcode.h"

#define STACKSIZE 2048
#define GLOBALS_SIZE 65536
#define MAX_FRAMES 1024

typedef enum vm_error_code {
    VM_ERROR_NONE,
    VM_STACKOVERFLOW,
    VM_UNSUPPORTED_OPERAND,
    VM_UNSUPPORTED_OPERATOR,
    VM_NON_FUNCTION,
    VM_WRONG_NUMBER_ARGUMENTS
} vm_error_code;

static const char *VM_ERROR_DESC[] = {
    "VM_ERROR_NONE",
    "STACKOVERFLOW",
    "UNSUPPORTED_OPERAND",
    "UNSUPPORTED_OPERATOR",
    "VM_NON_FUNCTION",
    "VM_WRONG_NUMBER_OF_ARGUMENTS"
};

typedef struct vm_error_t {
    vm_error_code code;
    char *msg;
} vm_error_t;

#define get_vm_error_desc(err) VM_ERROR_DESC[err]

typedef struct vm_t {
    frame_t *frames[MAX_FRAMES];
    size_t frame_index;
    cm_array_list *constants;
    monkey_object_t *stack[STACKSIZE];
    monkey_object_t *globals[GLOBALS_SIZE];
    size_t sp;
} vm_t;

vm_t *vm_init(bytecode_t *);
vm_t *vm_init_with_state(bytecode_t *, monkey_object_t *[GLOBALS_SIZE]);
void vm_free(vm_t *);
monkey_object_t *vm_last_popped_stack_elem(vm_t *);
vm_error_t vm_run(vm_t *);

#endif