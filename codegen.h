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

//! Type of operand of assembly instructions.
typedef enum asm_operand_type {
  //! Immediate (constant) value.
  ASM_OPND_IMM,
  //! Register.
  ASM_OPND_REG,
  //! Stack address.
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

//! Condition codes
typedef enum asm_cond_code {
  //! Equal
  CC_E,
  //! Not equal
  CC_NE,
  //! Less than
  CC_L,
  //! Less than or equal to
  CC_LE,
  //! Greater than
  CC_G,
  //! Greater than or equal to
  CC_GE,
} asm_cond_code;

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
  //! cmp
  ASM_CMP,
  //! Unconditional jump (jmp).
  ASM_JMP,
  //! Jump with conditions (jz, jnz, etc).
  ASM_JMPCC,
  //! Allocate stack space. Alias for subtracting the stack register.
  ASM_ALLOCSTACK,
  //! ret
  ASM_RETURN,
} asm_instruction_type;

typedef struct asm_instruction {
  //! Type of the assembly instruction.
  asm_instruction_type instruction_type;
  //! Source operand. The 1st operand from left to right (AT&T syntax).
  asm_operand* src;
  //! Destination operand. The 2nd operand from left to right (AT&T syntax).
  asm_operand* dst;
  //! Only used by jump instructions.
  const char* label;
  //! Only used by conditional jump instructions and conditional set
  //! instructions.
  asm_cond_code code;
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
