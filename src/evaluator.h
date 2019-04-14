#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "ast.h"
#include "environment.h"
#include "object.h"

monkey_object_t *monkey_eval(node_t *, environment_t *);
#endif