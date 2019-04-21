#ifndef BUILTINS_H
#define BUILTINS_H

#include "cmonkey_utils.h"
#include "object.h"

typedef cm_hash_table *monkey_builtins_table;

monkey_builtin_t *get_builtins(const char *);

#endif