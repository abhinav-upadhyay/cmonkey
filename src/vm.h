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
    VM_NON_FUNCTION
} vm_error_code;

static const char *VM_ERROR_DESC[] = {
    "VM_ERROR_NONE",
    "STACKOVERFLOW",
    "UNSUPPORTED_OPERAND",
    "UNSUPPORTED_OPERATOR",
    "VM_NON_FUNCTION"
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