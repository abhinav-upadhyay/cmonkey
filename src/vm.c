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

static frame_t *
get_current_frame(vm_t *vm)
{
    return vm->frames[vm->frame_index - 1];
}

static void
push_frame(vm_t *vm, frame_t *frame)
{
    vm->frames[vm->frame_index] = frame;
    vm->frame_index++;
}

static frame_t *
pop_frame(vm_t *vm)
{
    vm->frame_index--;
    return vm->frames[vm->frame_index];
}

vm_t *
vm_init(bytecode_t *bytecode)
{
    vm_t *vm;
    vm = malloc(sizeof(*vm));
    if (vm == NULL)
        err(EXIT_FAILURE, "malloc failed");
    monkey_compiled_fn_t *main_fn = create_monkey_compiled_fn(bytecode->instructions, 0, 0);
    frame_t *main_frame = frame_init(main_fn, 0);
    vm->frames[0] = main_frame;
    vm->frame_index = 1;
    vm->constants = bytecode->constants_pool;
    vm->sp = 0;
    for (size_t i = 0; i < GLOBALS_SIZE; i++)
        vm->globals[i] = NULL;
    free(main_fn);
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
    for (size_t i = 0; i < vm->frame_index; i++) {
        frame_free(vm->frames[i]);
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
execute_array_index_expression(vm_t *vm, monkey_array_t *left, monkey_int_t *index)
{
    vm_error_t vm_err = {VM_ERROR_NONE, NULL};
    if (index->value < 0 || index->value >= left->elements->length) {
        vm_push(vm, (monkey_object_t *) create_monkey_null(), false);
        return vm_err;
    }
    vm_push(vm, cm_array_list_get(left->elements, index->value), true);
    return vm_err;
}

static vm_error_t
execute_hash_index_expression(vm_t *vm, monkey_hash_t *left, monkey_object_t *index)
{
    vm_error_t vm_err = {VM_ERROR_NONE, NULL};
    monkey_object_t *value = cm_hash_table_get(left->pairs, index);
    if (value == NULL)
        vm_push(vm, (monkey_object_t *) create_monkey_null(), false);
    else
        vm_push(vm, value, true);
    return vm_err;
}

static vm_error_t
execute_index_expression(vm_t *vm, monkey_object_t *left, monkey_object_t *index)
{
    vm_error_t vm_err;
    if (left->type == MONKEY_ARRAY) {
        if (index->type != MONKEY_INT) {
            vm_err.code = VM_UNSUPPORTED_OPERATOR;
            vm_err.msg = get_err_msg("unsupported index operator type %s for array object",
                get_type_name(index->type));
            return vm_err;
        }
        return execute_array_index_expression(vm, (monkey_array_t *) left, (monkey_int_t *) index);
    } else if (left->type == MONKEY_HASH)
        return execute_hash_index_expression(vm, (monkey_hash_t *) left, index);
    vm_err.code = VM_UNSUPPORTED_OPERATOR;
    vm_err.msg = get_err_msg("index operator not supported for %s", get_type_name(left->type));
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

static cm_hash_table *
build_hash(vm_t *vm, size_t size)
{
    cm_hash_table *table = cm_hash_table_init(monkey_object_hash,
        monkey_object_equals, free_monkey_object, free_monkey_object);
    for (size_t i = vm->sp - size; i < vm->sp; i += 2) {
        monkey_object_t *key = (monkey_object_t *) vm->stack[i];
        monkey_object_t *value = (monkey_object_t *) vm->stack[i + 1];
        cm_hash_table_put(table, key, value);
    }
    vm->sp -= size;
    return table;
}

vm_error_t
vm_run(vm_t *vm)
{
    size_t const_index, jmp_pos, sym_index, array_size, hash_size;
    vm_error_t vm_err;
    opcode_definition_t op_def;
    monkey_object_t *top = NULL;
    cm_array_list *array_list;
    cm_hash_table *table;
    monkey_array_t *array_obj;
    monkey_hash_t *hash_obj;
    monkey_object_t *index;
    monkey_object_t *left;
    monkey_compiled_fn_t *compiled_fn;
    monkey_object_t *return_value;
    frame_t *new_frame;
    frame_t *popped_frame = NULL;
    size_t ip;
    size_t num_args;
    frame_t *current_frame = get_current_frame(vm);
    while (current_frame->ip < get_frame_instructions(current_frame)->length) {
        ip = current_frame->ip;
        instructions_t *current_frame_instructions = get_frame_instructions(current_frame);
        opcode_t op = current_frame_instructions->bytes[ip];
        if (top != NULL) {
            free_monkey_object(top);
            top = NULL;
        }
        switch (op) {
        case OPCONSTANT:
            const_index = decode_instructions_to_sizet(current_frame_instructions->bytes + ip + 1, 2);
            current_frame->ip += 2;
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
            jmp_pos = decode_instructions_to_sizet(current_frame_instructions->bytes + ip + 1, 2);
            current_frame->ip = jmp_pos - 1;
            break;
        case OPJMPFALSE:
            jmp_pos = decode_instructions_to_sizet(current_frame_instructions->bytes + ip + 1, 2);
            current_frame->ip += 2;
            top = vm_pop(vm);
            if (!is_truthy(top))
                current_frame->ip = jmp_pos - 1;
            break;
        case OPSETGLOBAL:
            sym_index = decode_instructions_to_sizet(current_frame_instructions->bytes + ip + 1, 2);
            current_frame->ip += 2;
            top = vm_pop(vm);
            vm->globals[sym_index] = copy_monkey_object(top);
            break;
        case OPSETLOCAL:
            sym_index = decode_instructions_to_sizet(current_frame_instructions->bytes + ip + 1, 1);
            current_frame->ip++;
            top = vm_pop(vm);
            vm->stack[current_frame->bp + sym_index] = copy_monkey_object(top);
            break;
        case OPGETGLOBAL:
            sym_index = decode_instructions_to_sizet(current_frame_instructions->bytes + ip + 1, 2);
            current_frame->ip += 2;
            vm_err = vm_push(vm, vm->globals[sym_index], true);
            if (vm_err.code != VM_ERROR_NONE)
                return vm_err;
            break;
        case OPGETLOCAL:
            sym_index = decode_instructions_to_sizet(current_frame_instructions->bytes + ip + 1, 1);
            current_frame->ip++;
            vm_err = vm_push(vm, vm->stack[current_frame->bp + sym_index], false);
            if (vm_err.code != VM_ERROR_NONE)
                return vm_err;
            break;
        case OPARRAY:
            array_size = decode_instructions_to_sizet(current_frame_instructions->bytes + ip + 1, 2);
            current_frame->ip += 2;
            array_list = build_array(vm, array_size);
            array_obj = create_monkey_array(array_list);
            vm_err = vm_push(vm, (monkey_object_t *) array_obj, false);
            if (vm_err.code != VM_ERROR_NONE)
                return vm_err;
            break;
        case OPHASH:
            hash_size = decode_instructions_to_sizet(current_frame_instructions->bytes + ip + 1, 2);
            current_frame->ip += 2;
            table = build_hash(vm, hash_size);
            hash_obj = create_monkey_hash(table);
            vm_err = vm_push(vm, (monkey_object_t *) hash_obj, false);
            if (vm_err.code != VM_ERROR_NONE)
                return vm_err;
            break;
        case OPINDEX:
            index = vm_pop(vm);
            left = vm_pop(vm);
            vm_err = execute_index_expression(vm, left, index);
            free_monkey_object(index);
            free_monkey_object(left);
            if (vm_err.code != VM_ERROR_NONE)
                return vm_err;
            break;
        case OPCALL:
            num_args = decode_instructions_to_sizet(current_frame_instructions->bytes + ip + 1, 1);
            top = vm->stack[vm->sp - 1 - num_args];
            current_frame->ip++;
            if (top->type != MONKEY_COMPILED_FUNCTION) {
                vm_err.code = VM_NON_FUNCTION;
                vm_err.msg = get_err_msg("Calling non-function\n");
                return vm_err;
            }
            compiled_fn = (monkey_compiled_fn_t *) top;
            if (compiled_fn->num_args != num_args) {
                vm_err.code = VM_WRONG_NUMBER_ARGUMENTS;
                vm_err.msg = get_err_msg("wrong number of arguments: want=%zu, got=%zu",
                    compiled_fn->num_args, num_args);
                return vm_err;
            }
            new_frame = frame_init(compiled_fn, vm->sp - num_args);
            push_frame(vm, new_frame);
            vm->sp = new_frame->bp + compiled_fn->num_locals;
            break;
        case OPRETURNVALUE:
            return_value = (monkey_object_t *) vm_pop(vm);
            popped_frame = pop_frame(vm);
            vm->sp = popped_frame->bp - 1;
            vm_push(vm, return_value, false);
            break;
        case OPRETURN:
            popped_frame = pop_frame(vm);
            vm->sp = popped_frame->bp - 1;
            vm_push(vm, (monkey_object_t *) create_monkey_null(), false);
            break;
        default:
            op_def = opcode_definition_lookup(op);
            vm_err.code = VM_UNSUPPORTED_OPERATOR;
            vm_err.msg = get_err_msg("Unsupported opcode %s", op_def.name);
            return vm_err;
        }
        if (popped_frame == current_frame) {
            frame_free(popped_frame);
            popped_frame = NULL;
        } else
            current_frame->ip++;
        current_frame = get_current_frame(vm);
    }
    vm_err.code = VM_ERROR_NONE;
    vm_err.msg = NULL;
    return vm_err;
}