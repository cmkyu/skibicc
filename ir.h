#ifndef SKIBICC_IR_H
#define SKIBICC_IR_H

#include <stdbool.h>

#include "parser.h"

typedef struct ir_val {
  bool is_constant;
  union {
    char* var_name;
    ast_node* constant;
  } val;
} ir_val;

typedef enum ir_instruction_type {
  IR_RETURN,
  IR_ARITH,
} ir_instruction_type;

typedef struct ir_instruction {
  ir_instruction_type instruction_type;
  ast_operator* op;
  // Populated if unary or binary `op`.
  ir_val* lhs;
  // Populated if binary `op`.
  ir_val* rhs;
  // Destination operand. NULL if not applicable to the instruction type.
  ir_val* dst;
} ir_instruction;

typedef struct ir_func_def {
  const char* name;
  array* instructions;
} ir_func_def;

typedef struct ir_node {
  ir_func_def* function_definition;
} ir_node;

ir_node* emit_ir(ast_node* ast);

void destroy_ir_node(ir_node* ir_node);

#endif  // SKIBICC_IR_H
