#ifndef SKIBICC_CODEGEN_H
#define SKIBICC_CODEGEN_H

#include <stdint.h>
#include <stdio.h>

#include "hashmap.h"
#include "ir.h"
#include "list.h"

typedef enum asm_register {
  EAX,
  R10D,
} asm_register;

typedef enum asm_operand_type {
  ASM_OPND_IMM,
  ASM_OPND_REG,
  ASM_OPND_STACK,
} asm_operand_type;

typedef struct asm_operand {
  asm_operand_type operand_type;
  union {
    // TODO: immediate might be uint64_t.
    int64_t immediate;
    asm_register reg;
    int64_t offset;
  } operand;
} asm_operand;

typedef enum asm_instruction_type {
  ASM_MOV,
  ASM_NEG,
  ASM_NOT,
  ASM_ALLOCSTACK,
  ASM_RETURN,
} asm_instruction_type;

typedef struct asm_instruction {
  asm_instruction_type instruction_type;
  asm_operand* src;
  asm_operand* dst;
} asm_instruction;

typedef struct asm_func_def {
  const char* name;
  list* instructions;
} asm_func_def;

typedef struct asm_node {
  asm_func_def* func_def;
} asm_node;

typedef struct stack_allocator {
  hashmap var_to_offset;
  int64_t offset;
} stack_allocator;

void emit(FILE* f, ir_node* ir);

#endif  // SKIBICC_CODEGEN_H
