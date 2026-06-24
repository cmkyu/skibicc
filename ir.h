#ifndef SKIBICC_IR_H
#define SKIBICC_IR_H

#include <stdbool.h>

#include "parser.h"
#include "strings.h"

typedef struct ir_val {
  bool is_constant;
  union {
    string_view var_name;
    ast_node* constant;
  } val;
  uint64_t ref_count;
} ir_val;

//! Represents the type of the IR instruction.
typedef enum ir_instruction_type {
  //! Return instruction.
  IR_RETURN,
  //! Unary instruction.
  IR_UNARY,
  //! Binary instruction.
  IR_BINARY,
  //! Unconditional jump instruction.
  IR_JMP,
  //! Jump if zero instruction.
  IR_JZ,
  //! Jump if not zero instruction.
  IR_JNZ,
  //! Label.
  IR_LABEL,
} ir_instruction_type;

//! Represents a single IR instruction.
typedef struct ir_instruction {
  ir_instruction_type instruction_type;
  //! Operator type. Only used for `IR_UNARY` and `IR_BINARY`.
  ast_operator* op;
  //! Left hand side. Or the "source" if the instruction only takes 1 operand.
  ir_val* lhs;
  //! Right hand side. Only used for `IR_BINARY`.
  ir_val* rhs;
  //! Destination operand.
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

void ir_destroy(ir_node* ir_node);

#endif  // SKIBICC_IR_H
