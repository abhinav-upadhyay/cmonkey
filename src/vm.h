#ifndef VM_H
#define VM_H

#include <stdlib.h>
#include "cmonkey_utils.h"
#include "compiler.h"
#include "object.h"
#include "opcode.h"

#define STACKSIZE 2048

typedef enum vm_error_code {
    VM_ERROR_NONE,
    VM_STACKOVERFLOW,
    VM_UNSUPPORTED_OPERAND,
    VM_UNSUPPORTED_OPERATOR
} vm_error_code;

static const char *VM_ERROR_DESC[] = {
    "VM_ERROR_NONE",
    "STACKOVERFLOW",
    "UNSUPPORTED_OPERAND",
    "UNSUPPORTED_OPERATOR"
};

typedef struct vm_error_t {
    vm_error_code code;
    char *msg;
} vm_error_t;

#define get_vm_error_desc(err) VM_ERROR_DESC[err]

typedef struct vm_t {
    cm_array_list *constants;
    instructions_t *instructions;
    monkey_object_t *stack[STACKSIZE];
    size_t sp;
} vm_t;

vm_t *vm_init(bytecode_t *);
void vm_free(vm_t *);
monkey_object_t *vm_last_popped_stack_elem(vm_t *);
vm_error_t vm_run(vm_t *);

#endif