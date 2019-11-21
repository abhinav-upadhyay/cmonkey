#include <err.h>
#include <stdlib.h>

#include "compiler.h"
#include "object.h"
#include "opcode.h"
#include "vm.h"

static char *
get_err_msg(const char *s, ...)
{
    char *msg = NULL;
    va_list ap;
    va_start(ap, s);
    int retval = vasprintf(&msg, s, ap);
    va_end(ap);
    if (retval == -1)
        err(EXIT_FAILURE, "malloc failed");
    return msg;
}


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
    for (size_t i = 0; i < GLOBALS_SIZE; i++)
        vm->globals[i] = NULL;
    return vm;
}

vm_t *
vm_init_with_state(bytecode_t *bytecode, monkey_object_t *globals[GLOBALS_SIZE])
{
    vm_t *vm = vm_init(bytecode);
    for (size_t i = 0; i < GLOBALS_SIZE; i++) {
        if (globals[i] != NULL)
            vm->globals[i] = copy_monkey_object(globals[i]);
        else
            break;
    }
    return vm;
}

void
vm_free(vm_t *vm)
{
    for (size_t i = 0; i < vm->sp; i++)
        free_monkey_object(vm->stack[i]);
    for (size_t i = 0; i < GLOBALS_SIZE; i++) {
        if (vm->globals[i] != NULL)
            free_monkey_object(vm->globals[i]);
        else
            break;
    }
    free(vm);
}

monkey_object_t *
vm_last_popped_stack_elem(vm_t *vm)
{
    return vm->stack[vm->sp];
}

static vm_error_t
vm_push(vm_t *vm, monkey_object_t *obj, _Bool copy)
{
    vm_error_t error = {VM_ERROR_NONE, NULL};
    if (vm->sp >= STACKSIZE) {
        error.code = VM_STACKOVERFLOW;
        error.msg = get_err_msg("Stackoverflow error: execeeded max stack size of %zu",
            STACKSIZE);
        return error;
    }
    vm->stack[vm->sp++] = copy? copy_monkey_object(obj): obj;
    return error;
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
    vm_error_t error = {VM_ERROR_NONE, NULL};
    opcode_definition_t op_def;
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
        op_def = opcode_definition_lookup(op);
        error.code = VM_UNSUPPORTED_OPERATOR;
        error.msg = get_err_msg("opcode %s not supported for integer operands", op_def.name);
        return error;
    }
    monkey_object_t *result_obj = (monkey_object_t *) create_monkey_int(result);
    vm_push(vm, result_obj, false);
    return error;
}

static vm_error_t
execute_binary_string_op(vm_t *vm, opcode_t op, monkey_string_t *leftval, monkey_string_t *rightval)
{
    char *result = NULL;
    vm_error_t error = {VM_ERROR_NONE, NULL};
    opcode_definition_t op_def;
    if (op != OPADD) {
        op_def = opcode_definition_lookup(op);
        error.code = VM_UNSUPPORTED_OPERATOR;
        error.msg = get_err_msg("opcode %s not support for string operands", op_def.name);
        return error;
    }
    if ((asprintf(&result, "%s%s", leftval->value, rightval->value)) == -1)
        err(EXIT_FAILURE, "malloc failed");
    monkey_object_t *result_obj = (monkey_object_t *) create_monkey_string(result, leftval->length + rightval->length);
    free(result);
    vm_push(vm, result_obj, false);
    return error;
}

static vm_error_t
execute_binary_op(vm_t *vm, opcode_t op)
{
    monkey_object_t *right = vm_pop(vm);
    monkey_object_t *left = vm_pop(vm);
    vm_error_t vm_err;
    if (left->type == MONKEY_INT && right->type == MONKEY_INT) {
        long leftval = ((monkey_int_t *) left)->value;
        long rightval = ((monkey_int_t *) right)->value;
        vm_err = execute_binary_int_op(vm, op, leftval, rightval);
    } else if (left->type == MONKEY_STRING && right->type == MONKEY_STRING) {
        vm_err = execute_binary_string_op(vm, op, (monkey_string_t *) left, (monkey_string_t *) right);
    }else {
        vm_err.code = VM_UNSUPPORTED_OPERAND;
        opcode_definition_t op_def = opcode_definition_lookup(op);
        vm_err.msg = get_err_msg("'%s' operation not supported with types %s and %s",
            op_def.desc, get_type_name(left->type), get_type_name(right->type));
    }
    free_monkey_object(left);
    free_monkey_object(right);
    return vm_err;
}

static vm_error_t
execute_integer_comparison(vm_t *vm, opcode_t op, long left, long right)
{
    _Bool result = false;
    vm_error_t error = {VM_ERROR_NONE, NULL};
    opcode_definition_t op_def;
    switch (op) {
    case OPGREATERTHAN:
        if (left > right)
            result = true;
        break;
    case OPEQUAL:
        if (left == right)
            result = true;
        break;
    case OPNOTEQUAL:
        if (left != right)
            result = true;
        break;
    default:
        op_def = opcode_definition_lookup(op);
        error.code = VM_UNSUPPORTED_OPERATOR;
        error.msg = get_err_msg("Unsupported opcode %s for integer operands", op_def.name);
        return error;
    }
    vm_push(vm, (monkey_object_t *) create_monkey_bool(result), false);
    return error;
}

static vm_error_t
execute_bang_operator(vm_t *vm)
{
    monkey_object_t *operand = vm_pop(vm);
    monkey_bool_t *bool_operand;
    vm_error_t vm_err;
    if (operand->type != MONKEY_BOOL && operand->type != MONKEY_NULL) {
        vm_err.code = VM_UNSUPPORTED_OPERAND;
        vm_err.msg = get_err_msg("'!' operator not supported for %s type operands",
            get_type_name(operand->type));
        return vm_err;
    }
    if (operand->type == MONKEY_NULL)
        bool_operand = create_monkey_bool(false);
    else
        bool_operand = (monkey_bool_t *) operand;
    vm_push(vm, (monkey_object_t *) create_monkey_bool(!bool_operand->value), false);
    vm_err.code = VM_ERROR_NONE;
    vm_err.msg = NULL;
    return vm_err;
}

static vm_error_t
execute_minus_operator(vm_t *vm)
{
    monkey_object_t *operand = vm_pop(vm);
    vm_error_t vm_err;
    if (operand->type != MONKEY_INT) {
        vm_err.code = VM_UNSUPPORTED_OPERAND;
        vm_err.msg = get_err_msg("'-' operator not supported for %s type operands",
            get_type_name(operand->type));
        return vm_err;
    }
    monkey_int_t *int_operand = (monkey_int_t *) operand;
    monkey_int_t *result = create_monkey_int(-int_operand->value);
    vm_push(vm, (monkey_object_t *) result, false);
    free_monkey_object(operand);
    vm_err.code = VM_ERROR_NONE;
    vm_err.msg = NULL;
    return vm_err;
}

static vm_error_t
execute_comparison_op(vm_t *vm, opcode_t op)
{
    vm_error_t error = {VM_ERROR_NONE, NULL};
    opcode_definition_t op_def;
    monkey_object_t *right = vm_pop(vm);
    monkey_object_t *left = vm_pop(vm);
    if (left->type == MONKEY_INT && right->type == MONKEY_INT) {
        long leftval = ((monkey_int_t *) left)->value;
        long rightval = ((monkey_int_t *) right)->value;
        error = execute_integer_comparison(vm, op, leftval, rightval);
    } else if (left->type == MONKEY_BOOL && right->type == MONKEY_BOOL) {
        _Bool result = false;
        switch (op) {
        case OPGREATERTHAN:
            vm_push(vm, (monkey_object_t *) create_monkey_bool(false), false);
            break;
        case OPEQUAL:
            if (left == right)
                result = true;
            break;
        case OPNOTEQUAL:
            if (left != right)
                result = true;
            break;
        default:
            op_def = opcode_definition_lookup(op);
            error.code = VM_UNSUPPORTED_OPERATOR;
            error.msg = get_err_msg("Unsupported opcode %s", op_def.name);
            goto RETURN;
        }
        vm_push(vm, (monkey_object_t *) create_monkey_bool(result), false);
    } else {
        error.code = VM_UNSUPPORTED_OPERAND;
        error.msg = get_err_msg("Unsupported operand types %s and %s",
            get_type_name(left->type), get_type_name(right->type));
    }
RETURN:
    free_monkey_object(left);
    free_monkey_object(right);
    return error;
}

static _Bool
is_truthy(monkey_object_t *condition)
{
    switch (condition->type) {
    case MONKEY_BOOL:
        return ((monkey_bool_t *) condition)->value;
    case MONKEY_NULL:
        return false;
    default:
        return true;
    }
}

static cm_array_list *
build_array(vm_t *vm, size_t array_size)
{
    cm_array_list *list = cm_array_list_init(array_size, free_monkey_object);
    for (size_t i = vm->sp - array_size; i < vm->sp; i++) {
        monkey_object_t *obj = (monkey_object_t *) vm->stack[i];
        cm_array_list_add(list, obj);
    }
    vm->sp -= array_size;
    return list;
}

vm_error_t
vm_run(vm_t *vm)
{
    size_t const_index, jmp_pos, sym_index, array_size;
    vm_error_t vm_err;
    opcode_definition_t op_def;
    monkey_object_t *top = NULL;
    cm_array_list *array_list;
    monkey_array_t *array_obj;
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
            vm_err = vm_push(vm, get_constant(vm, const_index), true);
            if (vm_err.code != VM_ERROR_NONE)
                return vm_err;
            break;
        case OPADD:
        case OPSUB:
        case OPMUL:
        case OPDIV:
            vm_err = execute_binary_op(vm, op);
            if (vm_err.code != VM_ERROR_NONE)
                return vm_err;
            break;
        case OPPOP:
            top = vm_pop(vm);
            break;
        case OPTRUE:
            vm_push(vm, (monkey_object_t *) create_monkey_bool(true), false);
            break;
        case OPFALSE:
            vm_push(vm, (monkey_object_t *) create_monkey_bool(false), false);
            break;
        case OPNULL:
            vm_push(vm, (monkey_object_t *) create_monkey_null(), false);
            break;
        case OPGREATERTHAN:
        case OPEQUAL:
        case OPNOTEQUAL:
            vm_err = execute_comparison_op(vm, op);
            if (vm_err.code != VM_ERROR_NONE)
                return vm_err;
            break;
        case OPMINUS:
            vm_err = execute_minus_operator(vm);
            if (vm_err.code != VM_ERROR_NONE)
                return vm_err;
            break;
        case OPBANG:
            vm_err = execute_bang_operator(vm);
            if (vm_err.code != VM_ERROR_NONE)
                return vm_err;
            break;
        case OPJMP:
            jmp_pos = decode_instructions_to_sizet(vm->instructions->bytes + ip + 1, 2);
            ip = jmp_pos - 1;
            break;
        case OPJMPFALSE:
            jmp_pos = decode_instructions_to_sizet(vm->instructions->bytes + ip + 1, 2);
            ip += 2;
            top = vm_pop(vm);
            if (!is_truthy(top))
                ip = jmp_pos - 1;
            break;
        case OPSETGLOBAL:
            sym_index = decode_instructions_to_sizet(vm->instructions->bytes + ip + 1, 2);
            ip += 2;
            top = vm_pop(vm);
            vm->globals[sym_index] = copy_monkey_object(top);
            break;
        case OPGETGLOBAL:
            sym_index = decode_instructions_to_sizet(vm->instructions->bytes + ip + 1, 2);
            ip += 2;
            vm_err = vm_push(vm, vm->globals[sym_index], true);
            if (vm_err.code != VM_ERROR_NONE)
                return vm_err;
            break;
        case OPARRAY:
            array_size = decode_instructions_to_sizet(vm->instructions->bytes + ip + 1, 2);
            ip += 2;
            array_list = build_array(vm, array_size);
            array_obj = create_monkey_array(array_list);
            vm_err = vm_push(vm, (monkey_object_t *) array_obj, false);
            if (vm_err.code != VM_ERROR_NONE)
                return vm_err;
            break;
        default:
            op_def = opcode_definition_lookup(op);
            vm_err.code = VM_UNSUPPORTED_OPERATOR;
            vm_err.msg = get_err_msg("Unsupported opcode %s", op_def.name);
            return vm_err;
        }
    }
    vm_err.code = VM_ERROR_NONE;
    vm_err.msg = NULL;
    return vm_err;
}