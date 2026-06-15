#include "codegen.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "errors.h"
#include "hashmap.h"
#include "ir.h"
#include "list.h"

static const char* reg_cx_to_string(asm_register_size reg_size) {
  switch (reg_size) {
    case _8L:
      return "cl";
    case _8H:
      return "ch";
    case _16:
      return "cx";
    case _32:
      return "ecx";
    case _64:
      return "rcx";
  }
  error("unexpected asm_register_type");
  return NULL;
}

static const char* asm_register_to_string(asm_register reg) {
  switch (reg.type) {
    case AX:
      return "eax";
    case CX:
      return reg_cx_to_string(reg.size);
    case DX:
      return "edx";
    case R10:
      return "r10d";
    case R11:
      return "r11d";
  }
  error("Unimplemented reg");
  return NULL;
}

static void stack_allocator_init(stack_allocator* alloc) {
  hashmap_init(&alloc->var_to_offset);
  alloc->offset = 0;
}

static inline bool stack_allocator_has_offset(stack_allocator* alloc) {
  return alloc->offset < 0;
}

static asm_operand* stack_allocator_get(stack_allocator* alloc, ir_val* val,
                                        int64_t offset) {
  asm_operand* opnd = calloc_safe(/*nelem=*/1, sizeof(asm_operand));
  opnd->operand_type = ASM_OPND_STACK;
  const char* var_name = val->val.var_name.data;
  size_t name_len = val->val.var_name.length;
  hashmap_entry* entry = hashmap_get(&alloc->var_to_offset, var_name, name_len);
  if (entry) {
    opnd->operand.offset = *((int64_t*)(entry->data));
    return opnd;
  }
  alloc->offset -= offset;
  opnd->operand.offset = alloc->offset;

  hashmap_entry new_entry;
  new_entry.key = strndup((char*)var_name, name_len);
  new_entry.key_size = name_len;
  int64_t* data = malloc_safe(sizeof(int64_t));
  *data = alloc->offset;
  new_entry.data = data;
  new_entry.data_size = sizeof(int64_t);
  hashmap_insert(&alloc->var_to_offset, &new_entry);
  return opnd;
}

static void stack_allocator_destroy(stack_allocator* alloc) {
  hashmap_destroy(&alloc->var_to_offset);
}

static asm_operand* create_register(asm_register_type reg_type,
                                    asm_register_size reg_size) {
  asm_operand* opnd = calloc_safe(/*nelem=*/1, sizeof(asm_operand));
  opnd->operand_type = ASM_OPND_REG;
  opnd->operand.reg.type = reg_type;
  opnd->operand.reg.size = reg_size;
  return opnd;
}

static asm_operand* create_immediate(int64_t immediate) {
  asm_operand* opnd = calloc_safe(/*nelem=*/1, sizeof(asm_operand));
  opnd->operand_type = ASM_OPND_IMM;
  opnd->operand.immediate = immediate;
  return opnd;
}

static void insert_allocate_stack_instruction(stack_allocator* alloc,
                                              list* instructions) {
  if (!stack_allocator_has_offset(alloc)) {
    return;
  }

  asm_instruction* inst = calloc_safe(/*nelem=*/1, sizeof(asm_instruction));
  inst->instruction_type = ASM_ALLOCSTACK;
  // x86 allocates stack using sub command, so we need to flip negative offset
  // to positive.
  inst->src = create_immediate(-alloc->offset);
  list_push_front(instructions, inst);
}

static void insert_mov(asm_operand* src, asm_operand* dst,
                       list* asm_instructions) {
  if (src->operand_type == ASM_OPND_STACK &&
      dst->operand_type == ASM_OPND_STACK) {
    // Both are stack addreses. Not allowed.
    // Need to rewrite from:
    // mov <stack1>, <stack2>
    // to:
    // mov <stack1>, %r10d
    // mov %r10d, <stack2>
    asm_instruction* inst = calloc_safe(/*nelem=*/1, sizeof(asm_instruction));
    inst->instruction_type = ASM_MOV;
    inst->src = src;
    inst->dst = create_register(R10, _32);
    list_push_back(asm_instructions, inst);
    // r10 should be the src of the next mov:
    // mov %r10d, <stack2>
    src = create_register(R10, _32);
  }

  asm_instruction* inst = calloc_safe(/*nelem=*/1, sizeof(asm_instruction));
  inst->instruction_type = ASM_MOV;
  inst->src = src;
  inst->dst = dst;
  list_push_back(asm_instructions, inst);
}

static void insert_ret(list* asm_instructions) {
  asm_instruction* inst = calloc_safe(/*nelem=*/1, sizeof(asm_instruction));
  inst->instruction_type = ASM_RETURN;
  list_push_back(asm_instructions, inst);
}

static void insert_neg(asm_operand* dst, list* asm_instructions) {
  asm_instruction* inst = calloc_safe(/*nelem=*/1, sizeof(asm_instruction));
  inst->instruction_type = ASM_NEG;
  inst->dst = dst;
  list_push_back(asm_instructions, inst);
}

static void insert_not(asm_operand* dst, list* asm_instructions) {
  asm_instruction* inst = calloc_safe(/*nelem=*/1, sizeof(asm_instruction));
  inst->instruction_type = ASM_NOT;
  inst->dst = dst;
  list_push_back(asm_instructions, inst);
}

static asm_operand* dup_operand(asm_operand* opnd) {
  asm_operand* res = malloc_safe(sizeof(asm_operand));
  memcpy(res, opnd, sizeof(asm_operand));
  return res;
}

static void lower_add(asm_operand* lhs, asm_operand* rhs, asm_operand* dst,
                      list* asm_instructions) {
  // mov lhs, dst
  insert_mov(lhs, dst, asm_instructions);
  dst = dup_operand(dst);
  if (rhs->operand_type == ASM_OPND_STACK &&
      dst->operand_type == ASM_OPND_STACK) {
    // Both are stack addreses. Not allowed. Mov rhs to r10.
    insert_mov(rhs, create_register(R10, _32), asm_instructions);
    rhs = create_register(R10, _32);
  }

  asm_instruction* inst = calloc_safe(/*nelem=*/1, sizeof(asm_instruction));
  inst->instruction_type = ASM_ADD;
  inst->src = rhs;
  inst->dst = dst;
  list_push_back(asm_instructions, inst);
}

static void lower_sub(asm_operand* lhs, asm_operand* rhs, asm_operand* dst,
                      list* asm_instructions) {
  // mov lhs, dst
  insert_mov(lhs, dst, asm_instructions);
  dst = dup_operand(dst);
  if (rhs->operand_type == ASM_OPND_STACK &&
      dst->operand_type == ASM_OPND_STACK) {
    // Both are stack addreses. Not allowed. Mov src to r10.
    insert_mov(rhs, create_register(R10, _32), asm_instructions);
    rhs = create_register(R10, _32);
  }

  asm_instruction* inst = calloc_safe(/*nelem=*/1, sizeof(asm_instruction));
  inst->instruction_type = ASM_SUB;
  inst->src = rhs;
  inst->dst = dst;
  list_push_back(asm_instructions, inst);
}

static void lower_mul(asm_operand* lhs, asm_operand* rhs, asm_operand* dst,
                      list* asm_instructions) {
  // mov lhs, dst
  insert_mov(lhs, dst, asm_instructions);
  const bool need_fixup = dst->operand_type == ASM_OPND_STACK;
  if (need_fixup) {
    // imull does not allow memory address as destination. move to r11.
    insert_mov(dup_operand(dst), create_register(R11, _32), asm_instructions);
  }

  asm_instruction* inst = calloc_safe(/*nelem=*/1, sizeof(asm_instruction));
  inst->instruction_type = ASM_MUL;
  inst->src = rhs;
  inst->dst = need_fixup ? create_register(R11, _32) : dup_operand(dst);
  list_push_back(asm_instructions, inst);
  if (need_fixup) {
    // mov r11 back to dst.
    insert_mov(create_register(R11, _32), dup_operand(dst), asm_instructions);
  }
}

static void lower_div(asm_operand* lhs, asm_operand* rhs, asm_operand* dst,
                      list* asm_instructions, bool want_remainder) {
  // mov lhs, eax
  insert_mov(lhs, create_register(AX, _32), asm_instructions);
  // cdq
  asm_instruction* inst = calloc_safe(/*nelem=*/1, sizeof(asm_instruction));
  inst->instruction_type = ASM_CDQ;
  list_push_back(asm_instructions, inst);

  if (rhs->operand_type == ASM_OPND_IMM) {
    // idiv does not work on immediate. mov rhs to r10.
    insert_mov(rhs, create_register(R10, _32), asm_instructions);
    rhs = create_register(R10, _32);
  }
  // idiv rhs
  inst = calloc_safe(/*nelem=*/1, sizeof(asm_instruction));
  inst->instruction_type = ASM_DIV;
  inst->dst = rhs;
  list_push_back(asm_instructions, inst);
  // division: mov eax, dst
  // modulo: mov edx, dst
  insert_mov(create_register(want_remainder ? DX : AX, _32), dst,
             asm_instructions);
}

static void lower_bit_shift(asm_operand* lhs, asm_operand* rhs,
                            asm_operand* dst, list* asm_instructions,
                            bool is_left) {
  // mov lhs, dst
  insert_mov(lhs, dst, asm_instructions);
  if (rhs->operand_type != ASM_OPND_IMM) {
    // Not an immmediate. mov to cl.
    insert_mov(rhs, create_register(CX, _8L), asm_instructions);
    rhs = create_register(CX, _8L);
  }

  asm_instruction* inst = calloc_safe(/*nelem=*/1, sizeof(asm_instruction));
  inst->instruction_type = (is_left ? ASM_SHL : ASM_SHR);
  inst->src = rhs;
  inst->dst = dup_operand(dst);
  list_push_back(asm_instructions, inst);
}

static asm_operand* lower_ir_val(ir_val* ir_val, stack_allocator* alloc) {
  asm_operand* opnd = NULL;
  if (!ir_val->is_constant) {
    // TODO: Type checking required for the stack offset.
    opnd = stack_allocator_get(alloc, ir_val, /*offset=*/4);
  } else {
    // TODO: this is extremely dumb and probably wrong. Fix it.
    opnd = create_immediate(
        ir_val->val.constant->node.consant->tok->constant.int_val);
  }
  return opnd;
}

static void lower_ir_return(ir_instruction* ir_instruction,
                            stack_allocator* alloc, list* asm_instructions) {
  // mov(val, reg(ax))
  asm_operand* src = lower_ir_val(ir_instruction->lhs, alloc);
  asm_operand* dst = create_register(AX, _32);
  insert_mov(src, dst, asm_instructions);

  // ret
  insert_ret(asm_instructions);
}

static void lower_ir_unary(ir_instruction* ir_instruction,
                           stack_allocator* alloc, list* asm_instructions) {
  // mov(src, dst)
  asm_operand* src = lower_ir_val(ir_instruction->lhs, alloc);
  asm_operand* dst = lower_ir_val(ir_instruction->dst, alloc);
  insert_mov(src, dst, asm_instructions);

  asm_operand* dst_copy = dup_operand(dst);
  switch (ir_instruction->op->op_type) {
    case OP_NEG:
      insert_neg(dst_copy, asm_instructions);
      break;
    case OP_BITNOT:
      insert_not(dst_copy, asm_instructions);
      break;
    default:
      error("Unimplemented unary");
  }
}

static void lower_ir_binary(ir_instruction* ir_instruction,
                            stack_allocator* alloc, list* asm_instructions) {
  // mov(lhs, dst)
  asm_operand* lhs = lower_ir_val(ir_instruction->lhs, alloc);
  asm_operand* rhs = lower_ir_val(ir_instruction->rhs, alloc);
  asm_operand* dst = lower_ir_val(ir_instruction->dst, alloc);
  switch (ir_instruction->op->op_type) {
    case OP_ADD:
      lower_add(lhs, rhs, dst, asm_instructions);
      break;
    case OP_SUB:
      lower_sub(lhs, rhs, dst, asm_instructions);
      break;
    case OP_MUL:
      lower_mul(lhs, rhs, dst, asm_instructions);
      break;
    case OP_DIV:
      lower_div(lhs, rhs, dst, asm_instructions, /*want_remainder=*/false);
      break;
    case OP_MOD:
      lower_div(lhs, rhs, dst, asm_instructions, /*want_remainder=*/true);
      break;
    case OP_SHL:
      lower_bit_shift(lhs, rhs, dst, asm_instructions, /*is_left=*/true);
      break;
    case OP_SHR:
      lower_bit_shift(lhs, rhs, dst, asm_instructions, /*is_left=*/false);
      break;
    default:
      error("Unimplemented binary");
  }
}

static void lower_ir_instruction(ir_instruction* ir_instruction,
                                 stack_allocator* alloc,
                                 list* asm_instructions) {
  switch (ir_instruction->instruction_type) {
    case IR_RETURN:
      lower_ir_return(ir_instruction, alloc, asm_instructions);
      break;
    case IR_UNARY:
      // TODO: just unary for now. Add more in the future.
      lower_ir_unary(ir_instruction, alloc, asm_instructions);
      break;
    case IR_BINARY:
      lower_ir_binary(ir_instruction, alloc, asm_instructions);
  }
}

static asm_func_def* lower_ir_func_def(ir_func_def* ir_func_def) {
  asm_func_def* func_def = calloc_safe(/*nelem=*/1, sizeof(asm_func_def));
  func_def->name = ir_func_def->name;
  func_def->instructions = list_init();

  stack_allocator alloc;
  stack_allocator_init(&alloc);

  for (size_t i = 0; i < ir_func_def->instructions->size; ++i) {
    ir_instruction* ir_inst = array_at(ir_func_def->instructions, i);
    lower_ir_instruction(ir_inst, &alloc, func_def->instructions);
  }

  insert_allocate_stack_instruction(&alloc, func_def->instructions);
  stack_allocator_destroy(&alloc);
  return func_def;
}

static asm_node* lower_ir(ir_node* ir) {
  asm_node* node = calloc_safe(/*nelem=*/1, sizeof(asm_node));
  node->func_def = lower_ir_func_def(ir->function_definition);
  return node;
}

static void println(FILE* f, char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(f, fmt, args);
  va_end(args);
  fprintf(f, "\n");
}

static char* emit_asm_operand(asm_operand* asm_operand) {
  if (!asm_operand) {
    return NULL;
  }

  size_t size;
  char* str = NULL;
  FILE* f = open_memstream(&str, &size);
  if (!f) {
    error("FATAL: emit_asm_operand(): open_memstream() failed.");
  }

  switch (asm_operand->operand_type) {
    case ASM_OPND_IMM:
      fprintf(f, "$%" PRId64, asm_operand->operand.immediate);
      break;
    case ASM_OPND_STACK:
      fprintf(f, "%" PRId64 "(%%rbp)", asm_operand->operand.offset);
      break;
    case ASM_OPND_REG:
      fprintf(f, "%%%s", asm_register_to_string(asm_operand->operand.reg));
      break;
  }
  fclose(f);
  return str;
}

static void emit_asm_instruction(FILE* f, asm_instruction* asm_instruction) {
  char* src = emit_asm_operand(asm_instruction->src);
  char* dst = emit_asm_operand(asm_instruction->dst);
  switch (asm_instruction->instruction_type) {
    case ASM_MOV:
      println(f, "movl %s, %s", src, dst);
      break;
    case ASM_NEG:
      println(f, "negl %s", dst);
      break;
    case ASM_NOT:
      println(f, "notl %s", dst);
      break;
    case ASM_ADD:
      println(f, "addl %s, %s", src, dst);
      break;
    case ASM_SUB:
      println(f, "subl %s, %s", src, dst);
      break;
    case ASM_MUL:
      println(f, "imull %s, %s", src, dst);
      break;
    case ASM_DIV:
      println(f, "idivl %s", dst);
      break;
    case ASM_CDQ:
      println(f, "cdq");
      break;
    case ASM_SHL:
      println(f, "shll %s, %s", src, dst);
      break;
    case ASM_SHR:
      println(f, "shrl %s, %s", src, dst);
      break;
    case ASM_ALLOCSTACK:
      println(f, "subq %s, %%rsp", src);
      break;
    case ASM_RETURN:
      println(f, "movq %%rbp, %%rsp");
      println(f, "popq %%rbp");
      println(f, "ret");
      break;
  }
  free(src);
  free(dst);
}

static void emit_asm_func_def(FILE* f, asm_func_def* asm_func_def) {
  println(f, ".globl %s", asm_func_def->name);
  println(f, "%s:", asm_func_def->name);
  println(f, "pushq %%rbp");
  println(f, "movq %%rsp, %%rbp");
  for (list_node* inst = asm_func_def->instructions->head; inst != NULL;
       inst = inst->next) {
    emit_asm_instruction(f, inst->data);
  }
}

static void destroy_asm_instruction(void* data) {
  asm_instruction* inst = (asm_instruction*)data;
  free(inst->src);
  free(inst->dst);
  free(inst);
}

static void destroy_asm_func_def(asm_func_def* asm_func_def) {
  // We don't free name here because it's owned by IR node.
  list_destroy(asm_func_def->instructions, destroy_asm_instruction);
  free(asm_func_def);
}

static void destroy_asm_node(asm_node* node) {
  destroy_asm_func_def(node->func_def);
  free(node);
}

void emit(FILE* f, ir_node* ir) {
  asm_node* node = lower_ir(ir);
  emit_asm_func_def(f, node->func_def);
  println(f, ".section .note.GNU-stack,\"\",@progbits");
  destroy_asm_node(node);
}
