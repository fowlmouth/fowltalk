#ifndef FT_DEBUG_BYTECODE_BUILDER
#define FT_DEBUG_BYTECODE_BUILDER 0
#endif

#define FT_DEBUG FT_DEBUG_BYTECODE_BUILDER
#include "fowltalk/debug.h"

#include "bytecode-builder.h"


void ft_init_bytecode_builder(struct ft_bytecode_builder* bb, struct ft_environment* env, oop bytecode, enum ft_bytecode_builder_flags flags, struct ft_vtable_slot_desc* slot_buffer) // struct ft_compiler_variable* var_buffer)
{
  (void)env; // unused param
  bb->bytecode = bytecode;
  bb->ip = 0;
  bb->flags = flags;
  bb->stack_size = bb->stack_max = 0;
  bb->slots_begin = bb->slots_end = slot_buffer;
}

void ft_init_bytecode_builder_object_context(struct ft_bytecode_builder* bb, struct ft_environment* env, struct ft_bytecode_builder* parent, struct ft_vtable_slot_desc* slot_buffer) //, struct ft_compiler_variable* var_buffer)
{
  ft_init_bytecode_builder(bb, env, parent->bytecode, ftbbf_object_context, slot_buffer);
  bb->ip = parent->ip;
}


int ft_bb_set_flags(struct ft_bytecode_builder* bb, int flags)
{
  return bb->flags = flags;
}

int ft_bb_get_flags(struct ft_bytecode_builder* bb)
{
  return bb->flags;
}

int ft_bb_add_literal(struct ft_bytecode_builder* bb, struct ft_environment* env, oop value)
{
  struct ft_array* literals = (struct ft_array*)bb->bytecode[1];
  int literals_size = ft_array_size(literals);
  oop* literal_items = ft_array_items_begin(literals);
  int i = 0;
  for(; i < literals_size; ++i)
  {
    if(!literal_items[i])
    {
      literal_items[i] = value;
      return i;
    }
    if(literal_items[i] == value)
    {
      return i;
    }
  }
  // literals is full, replace it
  int new_index = ft_array_size(literals);
  struct ft_array* new_literals = ft_new_array(ft_environment_allocator(env), ft_object_vtable(literals), new_index * 2);
  memcpy(ft_array_items_begin(new_literals), ft_array_items_begin(literals), sizeof(oop) * new_index);
  ft_array_items_begin(new_literals)[new_index] = value;
  bb->bytecode[1] = new_literals;
  return new_index;
}


void ft_bb_write_bytes(struct ft_bytecode_builder* bb, struct ft_environment* env, int count, ft_instruction_t* data)
{
  struct ft_bytes* instructions = (struct ft_bytes*)bb->bytecode[0];
  int minimum_size = bb->ip + count;
  if(ft_bytes_size(instructions) < minimum_size)
  {
    // resize instructions
    int new_size = ft_bytes_size(instructions) * 2;
    if(!new_size)
      // size is 0?
      new_size = 2;
    while(new_size < minimum_size)
    {
      new_size *= 2;
    }
    struct ft_bytes* new_instructions = ft_new_bytes(ft_environment_allocator(env), ft_object_vtable(instructions), new_size);
    memcpy(ft_bytes_data_begin(new_instructions), ft_bytes_data_begin(instructions), bb->ip);
    instructions = new_instructions;
    bb->bytecode[0] = instructions;
  }

  ft_debug("writing %d bytes at ip= %d", count, bb->ip);
  memcpy(ft_bytes_data_begin(instructions) + bb->ip, data, count);
  bb->ip += count;
}

void ft_bb_debug(struct ft_bytecode_builder* bb)
{
  ft_debug("stack size = %d  stack max = %d  bb = %p", bb->stack_size, bb->stack_max, bb);
}

void ft_bb_write_instruction(struct ft_bytecode_builder* bb, struct ft_environment* env, enum ft_vm_instruction instruction, int arg)
{
  const int instructions_bytes = 16;
  ft_instruction_t instructions[instructions_bytes];
  ft_instruction_t* ip = instructions + instructions_bytes;
  const int arg_shift = CHAR_BIT - ftvmi__bits;
  *--ip = instruction | ((arg & 0xFF) << ftvmi__bits);
  arg >>= arg_shift;
  while(arg && ip > instructions)
  {
    *--ip = ftvmi_extend | ((arg & 0xFF) << ftvmi__bits);
    arg >>= arg_shift;
  }

  // ft_debug("%x %x %x %x %x %x %x %x ", instructions[0], instructions[1], instructions[2], instructions[3], instructions[4], instructions[5], instructions[6], instructions[7]);

  ft_bb_write_bytes(bb, env, instructions_bytes - (ip - instructions), ip); // instructions + 8 - ip);
  ft_bb_debug(bb);
}

int ft_bb_pop_instruction(struct ft_bytecode_builder* bb, struct ft_environment* env, enum ft_vm_instruction* instruction, int* arg)
{
  (void)env; // unused
  if(!bb->ip) return 0;

  int popped = 0;
  struct ft_bytes* instructions = (struct ft_bytes*)bb->bytecode[0];
  const uint8_t* iseq = ft_bytes_data_begin(instructions);
  int tmp = iseq[ --bb->ip ];
  enum ft_vm_instruction tmp_op = tmp >> ftvmi__bits;
  *instruction = tmp & ftvmi__mask;
  *arg = tmp_op;
  ft_debug("op= %d  arg= %d", *instruction, *arg);
  ++popped;
  while(bb->ip)
  {
    tmp = iseq[ bb->ip - 1 ];
    tmp_op = tmp & ftvmi__mask;
    if(tmp_op != ftvmi_extend)
      return popped;
    *arg = *arg << (CHAR_BIT - ftvmi__bits) | (tmp >>  ftvmi__bits);
    ft_debug("arg= %d", *arg);
    ++popped;
    --bb->ip;
  }
  return popped;
}


#define STACK_MIN_EFFECT(min, operation)         \
  do{                                            \
    if(bb->stack_size < (min))                   \
      return ftbbec_stack_underflow;             \
    bb->stack_size = (bb->stack_size operation); \
    if(bb->stack_max < bb->stack_size)           \
      bb->stack_max = bb->stack_size;            \
    return ftbbec_none;                          \
  }while(0)

enum ft_bytecode_builder_error_code ft_bb_x_literal(struct ft_bytecode_builder* bb, struct ft_environment* env, oop value)
{
  int index = ft_bb_add_literal(bb, env, value);
  ft_debug("bb x literal(%d)", index);
  ft_bb_write_instruction(bb, env, ftvmi_literal, index);

  STACK_MIN_EFFECT(0, +1);
}

enum ft_bytecode_builder_error_code ft_bb_x_send(struct ft_bytecode_builder* bb, struct ft_environment* env, int argc, const char* selector)
{
  struct ft_string* str = ft_environment_intern(env, selector);
  int index = ft_bb_add_literal(bb, env, str);
  ft_debug("bb x send index= %d argc = %d, selector= '%s'", index, argc, selector);
  ft_bb_write_instruction(bb, env, ftvmi_literal, index);
  ft_bb_write_instruction(bb, env, ftvmi_send, argc);

  STACK_MIN_EFFECT(argc, +1-argc);
}

enum ft_bytecode_builder_error_code ft_bb_x_self_send(struct ft_bytecode_builder* bb, struct ft_environment* env, int argc, const char* selector)
{
  struct ft_string* str = ft_environment_intern(env, selector);
  int index = ft_bb_add_literal(bb, env, str);
  ft_debug("bb x self send index= %d argc = %d selector= '%s'", index, argc, selector);
  ft_bb_write_instruction(bb, env, ftvmi_literal, index);
  ft_bb_write_instruction(bb, env, ftvmi_self_send, argc);

  STACK_MIN_EFFECT(argc, +1-argc);
}

enum ft_bytecode_builder_error_code ft_bb_x_return(struct ft_bytecode_builder* bb, struct ft_environment* env, int scopes)
{
  ft_debug("bb x return scopes = %d", scopes);
  assert(scopes >= 0);
  ft_bb_write_instruction(bb, env, ftvmi_return, scopes);

  STACK_MIN_EFFECT(1, -1);
}

enum ft_bytecode_builder_error_code ft_bb_x_pop(struct ft_bytecode_builder* bb, struct ft_environment* env, int count)
{
  ft_debug("bb x pop count = %d", count);
  assert(count > 0);
  ft_bb_write_instruction(bb, env, ftvmi_pop, count);

  STACK_MIN_EFFECT(count, -count);
}

enum ft_bytecode_builder_error_code ft_bb_x_push_context(struct ft_bytecode_builder* bb, struct ft_environment* env)
{
  ft_debug("bb x push context");
  ft_bb_write_instruction(bb, env, ftvmi_push_context, 0);

  STACK_MIN_EFFECT(0, +1);
}

enum ft_bytecode_builder_error_code ft_bb_x_init_slot(struct ft_bytecode_builder* bb, struct ft_environment* env, int slot)
{
  ft_debug("bb x init slot = %d", slot);
  ft_bb_debug(bb);
  assert(slot >= 0);
  ft_bb_write_instruction(bb, env, ftvmi_init_slot, slot);

  STACK_MIN_EFFECT(2, -1);
}

#undef STACK_MIN_EFFECT

int ft_bb_slot_count(struct ft_bytecode_builder* bb)
{
  return bb->slots_end - bb->slots_begin;
}

int ft_bb_context_type(struct ft_bytecode_builder* bb)
{
  return bb->flags & ftbbf_context_mask;
}

// returns the index of the variable taking `self` and `lexicalParent` into account which aren't here as variables yet
// TODO take methods into account here, allow for local methods to be defined
int ft_bb_local_variable_index(struct ft_bytecode_builder* bb, int index)
{
  int context_type = ft_bb_context_type(bb);
  switch(context_type)
  {
  case ftbbf_method_context:
  case ftbbf_closure_context:
    return index;

  case ftbbf_object_context:
    return index;

  }
  return -1;
}

enum ft_bytecode_builder_error_code ft_bb_init_local(struct ft_bytecode_builder* bb, struct ft_environment* env, int index)
{
  ft_debug("bb init local index= %d", index);
  assert(index >= 0);
  assert(index < ft_bb_slot_count(bb));
  enum ft_bytecode_builder_error_code result;
  if((result = ft_bb_x_push_context(bb, env)))
    return result;
  return ft_bb_x_init_slot(bb, env, ft_bb_local_variable_index(bb, index));
}


int ft_bb_add_slot(struct ft_bytecode_builder* bb, struct ft_environment* env, const char* name, enum ft_vtable_slot_type type, oop value)
{
  struct ft_string* selector = ft_environment_intern(env, name);
  // struct ft_compiler_variable* var = bb->vars_end++;
  struct ft_vtable_slot_desc* slot = bb->slots_end++;
  ft_vtable_slot_desc_init(slot, type, selector, value);
  return slot - bb->slots_begin;
}

// returns the variable index of the slot with index `index`
int ft_bb_variable_index(struct ft_bytecode_builder* bb, int index)
{
  int result = 0;
  for(int i = 0; i < index; ++i)
  {
    switch(bb->slots_begin[i].type)
    {
    case ftvs_data_slot:
    case ftvs_parent_slot:
      ++result;
      break;
    default:
      break;
    }
  }
  return result;
}

int ft_bb_add_variable(struct ft_bytecode_builder* bb, struct ft_environment* env, const char* name, bool is_mutable, bool is_parent)
{
  int result = ft_bb_variable_index(bb, ft_bb_add_slot(bb, env, name, (is_parent ? ftvs_parent_slot : ftvs_data_slot), OOP_NIL));
  if(is_mutable)
  {
    int len = strlen(name);
    char mutator_name[len+2];
    memcpy(mutator_name, name, len);
    mutator_name[len] = ':';
    mutator_name[len+1] = 0;
    ft_bb_add_slot(bb, env, mutator_name, ftvs_setter_slot, ft_environment_intern(env, name));
  }
  return result;
}



struct ft_string* ft_symbol_as_mutator(struct ft_environment* env, struct ft_string* sym)
{
  const char* bytes = ft_string_bytes(sym);
  const int size = ft_string_size(sym);
  if(size && bytes[size - 1] == ':')
    return NULL;

  char new_bytes[size+2];
  memcpy(new_bytes, bytes, size);
  new_bytes[size] = ':';
  new_bytes[size+1] = 0;
  return ft_environment_intern(env, new_bytes);
}


void ft_bb_each_variable(struct ft_bytecode_builder* bb, void(*callback)(struct ft_vtable_slot_desc*, void*), void* arg)
{
  for(struct ft_vtable_slot_desc* iter = bb->slots_begin, *end = bb->slots_end;
    iter != end;
    ++iter)
  {
    callback(iter, arg);
  }
}

void ft_bb_debug_variables(struct ft_bytecode_builder* bb)
{
  ft_debug("bb slotCount= %d", ft_bb_slot_count(bb));
  for(struct ft_vtable_slot_desc* iter = bb->slots_begin, *end = bb->slots_end;
    iter != end;
    ++iter)
  {
    ft_debug("  %s  type='%d'", ft_string_bytes(iter->name), iter->type);
  }
}

// * finish compiling an object constructor
// * create a vtable containing the methods and slots
// * put the vtable on the stack and instantiate it
// * use init_slot to initiate the slots, leaving the new object on the stack
void ft_bb_finish_object_context(struct ft_bytecode_builder* bb, struct ft_environment* env, struct ft_vtable* vtable_vt)
{
  // NOTE I used VLA here but I was seeing stack smashing
  // struct ft_vtable_slot_desc slots[1 + ft_bb_variable_count(bb)];
  struct ft_vtable_slot_desc* slots = bb->slots_begin; // (struct ft_vtable_slot_desc*)calloc(sizeof(struct ft_vtable_slot_desc), 1 + ft_bb_variable_count(bb));
  struct ft_vtable_slot_desc* slot = bb->slots_end; // slots;

  int data_vars = 0;
  for(struct ft_vtable_slot_desc* iter = bb->slots_begin, *end = bb->slots_end;
    iter != end;
    ++iter)
  {

    switch(iter->type)
    {
    // case ftcvt_mutable:
    //   ft_debug("slot name= '%s'", ft_string_bytes(iter->name));
    //   ft_vtable_slot_desc_init(slot++, ftvs_data_slot, iter->name, NULL);
    //   ft_vtable_slot_desc_init(slot++, ftvs_setter_slot, ft_symbol_as_mutator(env, iter->name), iter->name);
    //   ++data_vars;
    //   break;

    // case ftcvt_local:
    //   ft_debug("local slot name= '%s'", ft_string_bytes(iter->name));
    //   ft_vtable_slot_desc_init(slot++, ftvs_data_slot, iter->name, NULL);
    //   ++data_vars;
    //   break;

    // case ftcvt_method:
    //   ft_debug("method name= '%s'", ft_string_bytes(iter->name));
    //   ft_vtable_slot_desc_init(slot++, ftvs_static_slot, iter->name, iter->value);
    //   break;

    case ftvs_parent_slot:
    case ftvs_data_slot:
      ++data_vars;
      break;

    default:
      ft_debug("unhandled variable in object context! '%s' type= '%d'", ft_string_bytes(iter->name), (int)iter->type);
      break;

    }
  }

  struct ft_vtable* vt = ft_new_vtable(ft_environment_allocator(env), vtable_vt, slot - slots, slots, 0);
  ft_debug("vt= %p  data vars= %d", vt, data_vars);
  ft_bb_x_literal(bb, env, vt);
  ft_bb_x_send(bb, env, 1, "__VTable_instantiate");

  ft_bb_debug_variables(bb);

  struct ft_vtable_slot_desc* slot_ptr = bb->slots_end - 1;
  for(int i = data_vars; i--; )
  {
    while(slot_ptr->type != ftvs_data_slot && slot_ptr->type != ftvs_parent_slot) // var_ptr->type != ftcvt_local && var_ptr->type != ftcvt_mutable)
      --slot_ptr;
    // find the index of var_ptr in vt
    struct ft_vtable_slot* slot = ft_vtable_lookup(vt, slot_ptr->name);
    assert(ft_vtable_slot_type(slot) == ftvs_data_slot || ft_vtable_slot_type(slot) == ftvs_parent_slot);
    int slot_index = (int)OOP_INT(ft_vtable_slot_value(slot));
    ft_bb_x_init_slot(bb, env, slot_index);
    ft_debug("init slot '%s' '%s' (index= %d)",
      ft_string_bytes(slot_ptr->name),
      ft_string_bytes(ft_vtable_slot_name(slot)),
      slot_index);
    --slot_ptr;
  }

  // free(slots);
}

oop ft_bb_create_method(struct ft_bytecode_builder* bb, struct ft_environment* env, struct ft_vtable* vtable_vt)
{
  // after every statement a pop instruction is written, we likely need to replace the last instruction with a return
  enum ft_vm_instruction instr = 0;
  int arg = 0;
  if(ft_bb_pop_instruction(bb, env, &instr, &arg))
  {
    ft_debug("popped instuction op= %d arg= %d", instr, arg);
    if(instr == ftvmi_pop)
    {
      bb->stack_size += arg;
      if(arg > 1)
      {
        // write the pop back leaving one on the stack
        ft_bb_x_pop(bb, env, arg-1);
      }
    }
    else
    {
      // put it back because it wasn't a pop
      ft_bb_write_instruction(bb, env, instr, arg);
    }
  }
  ft_bb_x_return(bb, env, 0);

  ft_debug("finish method context distance for slots= %d", (int)(bb->slots_end - bb->slots_begin));

  const int bb_slot_count = bb->slots_end - bb->slots_begin;
  // const int total_slot_count = 2 + bb_var_count * 2 ; // multiply by 2 to allow for mutable slots


  // for(struct ft_vtable_slot_desc* iter = bb->slots_begin, *end = bb->slots_end;
  //   iter != end;
  //   ++iter)
  // {

  //   switch(iter->type)
  //   {
  //   case ftcvt_parent:
  //     ft_debug("parent slot name= '%s'", ft_string_bytes(iter->name));
  //     ft_vtable_slot_desc_init(slot++, ftvs_parent_slot, iter->name, NULL);
  //     break;

  //   case ftcvt_mutable:
  //     ft_debug("slot name = %s", ft_string_bytes(iter->name));
  //     ft_vtable_slot_desc_init(slot++, ftvs_data_slot, iter->name, NULL);
  //     ft_vtable_slot_desc_init(slot++, ftvs_setter_slot, ft_symbol_as_mutator(env, iter->name), iter->name);
  //     break;

  //   case ftcvt_local:
  //     ft_debug("slot name = %s", ft_string_bytes(iter->name));
  //     ft_vtable_slot_desc_init(slot++, ftvs_data_slot, iter->name, NULL);
  //     break;

  //   case ftcvt_method:
  //     ft_debug("method name= '%s'", ft_string_bytes(iter->name));
  //     ft_vtable_slot_desc_init(slot++, ftvs_static_slot, iter->name, iter->value);
  //     break;

  //   case ftcvt_static:
  //     ft_debug("static slot name= '%s", ft_string_bytes(iter->name));
  //     ft_vtable_slot_desc_init(slot++, ftvs_static_slot, iter->name, NULL);
  //     break;

  //   }

  // }

  struct ft_vtable* vt = ft_new_method_vtable(ft_environment_allocator(env), vtable_vt, bb->slots_end - bb->slots_begin, bb->slots_begin, bb->bytecode);

  oop method = ft_vtable_instantiate(ft_environment_allocator(env), vt);
  return method;
}


