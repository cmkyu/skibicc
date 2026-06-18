#ifndef SKIBICC_CODEGEN_H
#define SKIBICC_CODEGEN_H

#include <stdint.h>
#include <stdio.h>

#include "hashmap.h"
#include "ir.h"
#include "list.h"

//! Size of the register.
typedef enum asm_register_size {
  //! Low 8 bits.
  _8L,
  //! High 8 bits.
  _8H,
  //! 16 bits.
  _16,
  //! 32 bits.
  _32,
  //! 64 bits.
  _64,
} asm_register_size;

//! Type of the register.
typedef enum asm_register_type {
  //! rax
  AX,
  //! rcx
  CX,
  //! rdx
  DX,
  //! r10
  R10,
  //! r11
  R11,
} asm_register_type;

typedef struct asm_register {
  asm_register_type type;
  asm_register_size size;
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
  ASM_ADD,
  ASM_SUB,
  ASM_MUL,
  ASM_DIV,
  ASM_CDQ,
  ASM_SHL,
  ASM_SHR,
  ASM_AND,
  ASM_OR,
  ASM_XOR,
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
