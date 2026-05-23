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

const char* asm_register_to_string(asm_register reg) {
  switch (reg) {
    case EAX:
      return "eax";
    case R10D:
      return "r10d";
  }
  error("Unimplemented reg");
  return NULL;
}

const char* asm_instruction_type_to_string(asm_instruction_type inst_type) {
  switch (inst_type) {
    case ASM_MOV:
      return "movl";
    case ASM_NEG:
      return "negl";
    case ASM_NOT:
      return "notl";
    case ASM_ALLOCSTACK:
      return "subq";
    case ASM_RETURN:
      return "ret";
  }
  error("Unimplemented asm inst");
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
  // TODO: strlen is expensive here. Should be precomputed by `var_name`.
  size_t name_len = strlen(val->val.var_name);
  hashmap_entry* entry =
      hashmap_get(&alloc->var_to_offset, val->val.var_name, name_len);
  if (entry) {
    opnd->operand.offset = *((int64_t*)(entry->data));
    return opnd;
  }
  alloc->offset -= offset;
  opnd->operand.offset = alloc->offset;

  hashmap_entry new_entry;
  new_entry.key = strndup((char*)val->val.var_name, name_len);
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

static asm_operand* create_register(asm_register reg) {
  asm_operand* opnd = calloc_safe(/*nelem=*/1, sizeof(asm_operand));
  opnd->operand_type = ASM_OPND_REG;
  opnd->operand.reg = reg;
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
  asm_instruction* inst = calloc_safe(/*nelem=*/1, sizeof(asm_instruction));
  inst->instruction_type = ASM_MOV;
  inst->src = src;
  if (src->operand_type == ASM_OPND_STACK &&
      dst->operand_type == ASM_OPND_STACK) {
    // Both are stack addreses. Not allowed.
    // Need to rewrite from:
    // mov <stack1>, <stack2>
    // to:
    // mov <stack1>, %r10d
    // mov %r10d, <stack2>
    inst->dst = create_register(R10D);
    list_push_back(asm_instructions, inst);

    // %r10d should be the src of the next mov:
    // mov %r10d, <stack2>
    inst = calloc_safe(/*nelem=*/1, sizeof(asm_instruction));
    inst->instruction_type = ASM_MOV;
    inst->src = create_register(R10D);
  }
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
  asm_operand* dst = create_register(EAX);
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

  asm_operand* dst_copy = malloc_safe(sizeof(asm_operand));
  memcpy(dst_copy, dst, sizeof(asm_operand));
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

static void lower_ir_instruction(ir_instruction* ir_instruction,
                                 stack_allocator* alloc,
                                 list* asm_instructions) {
  switch (ir_instruction->instruction_type) {
    case IR_RETURN:
      lower_ir_return(ir_instruction, alloc, asm_instructions);
      break;
    case IR_ARITH:
      // TODO: just unary for now. Add more in the future.
      lower_ir_unary(ir_instruction, alloc, asm_instructions);
      break;
    default:
      error("Unimplemented ir inst");
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

static void emit_asm_operand(FILE* f, asm_operand* asm_operand) {
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
}

static void emit_asm_instruction(FILE* f, asm_instruction* asm_instruction) {
  switch (asm_instruction->instruction_type) {
    case ASM_MOV:
      fprintf(f, "movl ");
      emit_asm_operand(f, asm_instruction->src);
      fprintf(f, ", ");
      emit_asm_operand(f, asm_instruction->dst);
      fprintf(f, "\n");
      break;
    case ASM_RETURN:
      println(f, "movq %%rbp, %%rsp");
      println(f, "popq %%rbp");
      println(f, "ret");
      break;
    case ASM_ALLOCSTACK:
      fprintf(f, "subq ");
      emit_asm_operand(f, asm_instruction->src);
      fprintf(f, ", %%rsp\n");
      break;
    case ASM_NEG:
      fprintf(f, "negl ");
      emit_asm_operand(f, asm_instruction->dst);
      fprintf(f, "\n");
      break;
    case ASM_NOT:
      fprintf(f, "notl ");
      emit_asm_operand(f, asm_instruction->dst);
      fprintf(f, "\n");
      break;
  }
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
