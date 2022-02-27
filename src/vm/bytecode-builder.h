#ifndef H_FOWLTALK_BYTECODE_BUILDER
#define H_FOWLTALK_BYTECODE_BUILDER

#include "fowltalk.h"
#include "fowltalk/vm.h"
#include "fowltalk/array.h"
#include "fowltalk/string.h"
#include "fowltalk/bytes.h"
#include "fowltalk/environment.h"
#include "fowltalk/vtable.h"

#include <string.h>
#include <assert.h>
#include <limits.h>


// enum ft_compiler_variable_type
// {
//   ftcvt_local = 0,
//   ftcvt_static = 1 << 0,
//   ftcvt_parent = 1 << 1,
//   ftcvt_method = 1 << 2, // unused at the moment
//   ftcvt_mutable = 1 << 3,

//   ftcvt__bits = 4
// };

// struct ft_compiler_variable
// {
//   struct ft_string* name;
//   enum ft_compiler_variable_type type;
//   oop value;
// };

enum ft_bytecode_builder_flags
{
  ftbbf_object_context = 0b001,
  ftbbf_method_context,
  ftbbf_closure_context,

  ftbbf_last_statement_was_method = 0b100,

  ftbbf_context_mask = 0b011
};

enum ft_bytecode_builder_error_code
{
  ftbbec_none = 0,
  ftbbec_stack_underflow = 0x100,
  ftbbec_object_stack_error
};

struct ft_bytecode_builder
{
  ft_object bytecode;
  int flags, ip;
  int stack_size, stack_max;
  // struct ft_compiler_variable *vars_begin, *vars_end;
  struct ft_vtable_slot_desc *slots_begin, *slots_end;
};

void ft_init_bytecode_builder(struct ft_bytecode_builder* bb, struct ft_environment* env, oop bytecode, enum ft_bytecode_builder_flags flags, struct ft_vtable_slot_desc* slot_buffer); //, struct ft_compiler_variable* var_buffer);
void ft_init_bytecode_builder_object_context(struct ft_bytecode_builder* bb, struct ft_environment* env, struct ft_bytecode_builder* parent, struct ft_vtable_slot_desc* slot_buffer); //, struct ft_compiler_variable* var_buffer);


int ft_bb_get_flags(struct ft_bytecode_builder* bb);
int ft_bb_set_flags(struct ft_bytecode_builder* bb, int flags);
int ft_bb_add_literal(struct ft_bytecode_builder* bb, struct ft_environment* env, oop value);
int ft_bb_add_slot(struct ft_bytecode_builder* bb, struct ft_environment* env, const char* name, enum ft_vtable_slot_type type, oop value);
int ft_bb_add_variable(struct ft_bytecode_builder* bb, struct ft_environment* env, const char* name, bool is_mutable, bool is_parent);
void ft_bb_write_bytes(struct ft_bytecode_builder* bb, struct ft_environment* env, int count, ft_instruction_t* data);
void ft_bb_write_instruction(struct ft_bytecode_builder* bb, struct ft_environment* env, enum ft_vm_instruction instruction, int arg);
int ft_bb_pop_instruction(struct ft_bytecode_builder* bb, struct ft_environment* env, enum ft_vm_instruction* instruction, int* arg);

enum ft_bytecode_builder_error_code ft_bb_x_literal(struct ft_bytecode_builder* bb, struct ft_environment* env, oop value);
enum ft_bytecode_builder_error_code ft_bb_x_send(struct ft_bytecode_builder* bb, struct ft_environment* env, int argc, const char* selector);
enum ft_bytecode_builder_error_code ft_bb_x_self_send(struct ft_bytecode_builder* bb, struct ft_environment* env, int argc, const char* selector);
enum ft_bytecode_builder_error_code ft_bb_x_return(struct ft_bytecode_builder* bb, struct ft_environment* env, int scopes);
enum ft_bytecode_builder_error_code ft_bb_x_pop(struct ft_bytecode_builder* bb, struct ft_environment* env, int count);
enum ft_bytecode_builder_error_code ft_bb_x_push_context(struct ft_bytecode_builder* bb, struct ft_environment* env);
enum ft_bytecode_builder_error_code ft_bb_x_init_slot(struct ft_bytecode_builder* bb, struct ft_environment* env, int slot);

enum ft_bytecode_builder_error_code ft_bb_init_local(struct ft_bytecode_builder* bb, struct ft_environment* env, int index);

void ft_bb_finish_object_context(struct ft_bytecode_builder* bb, struct ft_environment* env, struct ft_vtable* vtable_vt);
oop ft_bb_create_method(struct ft_bytecode_builder* bb, struct ft_environment* env, struct ft_vtable* vtable_vt);

int ft_bb_slot_count(struct ft_bytecode_builder* bb);
int ft_bb_context_type(struct ft_bytecode_builder* bb);

#endif



