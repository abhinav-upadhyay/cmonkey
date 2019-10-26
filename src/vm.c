#include <err.h>
#include <stdlib.h>

#include "compiler.h"
#include "object.h"
#include "opcode.h"
#include "vm.h"

vm_t *
vm_init(bytecode_t *bytecode)
{
    vm_t *vm;
    vm = malloc(sizeof(*vm));
    if (vm == NULL)
        err(EXIT_FAILURE, "malloc failed");
    vm->instructions = bytecode->instructions;
    vm->constants = bytecode->constants_pool;
    vm->sp = 0;
    return vm;
}

void
vm_free(vm_t *vm)
{
    for (size_t i = 0; i < vm->sp; i++)
        free_monkey_object(vm->stack[i]);
    free(vm);
}

monkey_object_t *
vm_last_popped_stack_elem(vm_t *vm)
{
    return vm->stack[vm->sp];
}

static vm_error_t
vm_push(vm_t *vm, monkey_object_t *obj)
{
    if (vm->sp >= STACKSIZE)
        return VM_STACKOVERFLOW;
    vm->stack[vm->sp++] = copy_monkey_object(obj);
    return VM_ERROR_NONE;
}

static monkey_object_t *
vm_pop(vm_t *vm)
{
    monkey_object_t *obj = vm->stack[vm->sp - 1];
    vm->sp--;
    return obj;
}

static monkey_object_t *
get_constant(vm_t *vm, size_t const_index)
{
    return (monkey_object_t *) cm_array_list_get(vm->constants, const_index);
}

static vm_error_t
execute_binary_int_op(vm_t *vm, opcode_t op, long leftval, long rightval)
{
    long result;
    switch (op) {
    case OPADD:
        result = leftval + rightval;
        break;
    case OPSUB:
        result = leftval - rightval;
        break;
    case OPMUL:
        result = leftval * rightval;
        break;
    case OPDIV:
        result = leftval / rightval;
        break;
    default:
        return VM_UNSUPPORTED_OPERATOR;
    }
    monkey_object_t *result_obj = (monkey_object_t *) create_monkey_int(result);
    vm_push(vm, result_obj);
    free_monkey_object(result_obj);
    return VM_ERROR_NONE;
}

static vm_error_t
execute_binary_op(vm_t *vm, opcode_t op)
{
    monkey_object_t *right = vm_pop(vm);
    monkey_object_t *left = vm_pop(vm);
    long leftval = ((monkey_int_t *) left)->value;
    long rightval = ((monkey_int_t *) right)->value;
    vm_error_t vm_err = VM_UNSUPPORTED_OPERAND;
    if (left->type == MONKEY_INT && right->type == MONKEY_INT)
        vm_err = execute_binary_int_op(vm, op, leftval, rightval);
    free_monkey_object(left);
    free_monkey_object(right);
    return vm_err;
}

vm_error_t
vm_run(vm_t *vm)
{
    size_t const_index;
    vm_error_t vm_err;
    monkey_object_t *top = NULL;
    for (size_t ip = 0; ip < vm->instructions->length; ip++) {
        opcode_t op = vm->instructions->bytes[ip];
        if (top != NULL) {
            free_monkey_object(top);
            top = NULL;
        }
        switch (op) {
        case OPCONSTANT:
            const_index = decode_instructions_to_sizet(vm->instructions->bytes + ip + 1, 2);
            ip += 2;
            vm_err = vm_push(vm, get_constant(vm, const_index));
            if (vm_err != VM_ERROR_NONE)
                return vm_err;
            break;
        case OPADD:
        case OPSUB:
        case OPMUL:
        case OPDIV:
            vm_err = execute_binary_op(vm, op);
            if (vm_err != VM_ERROR_NONE)
                return vm_err;
            break;
        case OPPOP:
            top = vm_pop(vm);
            break;
        case OPTRUE:
            vm_push(vm, (monkey_object_t *) create_monkey_bool(true));
            break;
        case OPFALSE:
            vm_push(vm, (monkey_object_t *) create_monkey_bool(false));
            break;
        default:
            return VM_UNSUPPORTED_OPERATOR;
        }
    }
    return VM_ERROR_NONE;
}