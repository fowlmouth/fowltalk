#ifndef FT_DEBUG_COMPILER
#define FT_DEBUG_COMPILER 0
#endif

#define FT_DEBUG FT_DEBUG_COMPILER
#include "fowltalk/debug.h"
#include "fowltalk/compiler.h"
#include "fowltalk/parser.h"
#include "fowltalk/vm.h"

#include "object-header.h"
#include "bytecode-builder.h"

#include <stdbool.h>
#include <ctype.h>

struct ft_compiler
{
  enum ft_bytecode_builder_error_code error_code;
  struct ft_compiler_options* options;
  struct ft_environment* env;
  struct ft_bytecode_builder* context;
  struct ft_bytecode_builder context_stack[16];
  struct ft_vtable_slot_desc slot_buffer[1024];
};


bool ftc_should_continue(void* arg)
{
  struct ft_compiler* c = (struct ft_compiler*)arg;
  return c->error_code == ftbbec_none;
}

void ftc_accept_send(const char* selector, int argc, void* arg)
{
  ft_debug("[ftc] accept send = %s argc = %d", selector, argc);
  struct ft_compiler* c = (struct ft_compiler*)arg;
  c->error_code = ft_bb_x_send(c->context, c->env, argc, selector);
}

void ftc_accept_self_send(const char* selector, int argc, void* arg)
{
  ft_debug("[ftc] accept self send = %s argc = %d", selector, argc);
  struct ft_compiler* c = (struct ft_compiler*)arg;
  c->error_code = ft_bb_x_self_send(c->context, c->env, argc, selector);
}

void ftc_accept_integer(ft_integer_t integer, void* arg)
{
  ft_debug("[ftc] accept integer = %ld", (long)integer);
  struct ft_compiler* c = (struct ft_compiler*)arg;
  struct ft_bytecode_builder* bb = c->context;
  c->error_code = ft_bb_x_literal(bb, c->env, INT_OOP(integer));
  ft_debug("[ftc] ip = %d", bb->ip);
}

void ftc_accept_string(const char* str, void* arg)
{
  ft_debug("[ftc] accept string= '%s'", str);
  struct ft_compiler* c = (struct ft_compiler*)arg;
  struct ft_bytecode_builder* bb = c->context;
  int len = strlen(str);
  oop string_prototype = ft_simple_lookup(ft_environment_lobby(c->env), ft_environment_intern(c->env, "String"));
  struct ft_vtable* string_vt = ft_object_vtable(string_prototype);
  assert(string_vt);
  struct ft_string* string = ft_new_string(ft_environment_allocator(c->env),
    string_vt,
    len, str);
  c->error_code = ft_bb_x_literal(bb, c->env, string);
  ft_debug("[ftc] ip = %d", bb->ip);
}

void ftc_accept_assignment(const char* variable, const char* assignment_op, void* arg, int is_parent)
{
  // TODO because of the init_local requiring the index of the variable to set, the easiest way to ensure
  // things dont break is to require that parent slots be defined before any data slots, that way
  // the parent slots can have the correct index calculated
  // support is required in ft_bb_add_slot
  ft_debug("[ftc] accept assignment '%s' '%s' is_parent=%d", variable, assignment_op, is_parent);
  struct ft_compiler* c = (struct ft_compiler*)arg;
  struct ft_bytecode_builder* bb = c->context;
  bool mutable = strcmp(":=", assignment_op) == 0;
  int local_index = ft_bb_add_variable(bb, c->env, variable, mutable, is_parent);

  if(ft_bb_context_type(bb) != ftbbf_object_context)
  {
    c->error_code = ft_bb_init_local(bb, c->env, local_index);
  }
}

// void ftc_accept_parent_assignment(const char* variable, const char* assignment_op, void* arg)
// {
//   ft_debug("[ftc] accept parent assignment '%s' '%s'", variable, assignment_op);
//   struct ft_compiler* c = (struct ft_compiler*)arg;
//   struct ft_bytecode_builder* bb = c->context;
//   bool mutable = strcmp(":=", assignment_op) == 0;
//   int local_index = ft_bb_add_variable(bb, c->env, variable, mutable, 1);

//   if(ft_bb_context_type(bb) != ftbbf_object_context)
//   {
//     c->error_code = ft_bb_init_local(bb, c->env, local_index);
//   }
// }

void ftc_accept_identifier(const char* identifier, void* arg)
{
  ft_debug("[ftc] accept identifier '%s'", identifier);
  struct ft_compiler* c = (struct ft_compiler*)arg;
  struct ft_bytecode_builder* bb = c->context;
  c->error_code = ft_bb_x_self_send(bb, c->env, 0, identifier);
}

int ft_compiler_explicit_return_context_count(struct ft_compiler* co)
{
  struct ft_bytecode_builder* ctx = co->context;
  int count = 0;
  while(ctx >= co->context_stack)
  {
    ft_debug("  ctx= '%p'  count= '%d'  context type= '%d'",
      ctx,
      count,
      (int)ft_bb_context_type(ctx));
    switch(ft_bb_context_type(ctx))
    {
    case ftbbf_method_context:
      return count;

    case ftbbf_closure_context:
      ++count;
      --ctx;
      break;

    case ftbbf_object_context:
      --ctx;
      break;
    }
  }
  // TODO this should be unreachable
  abort();
  return -1;
}

void ftc_accept_return(void* arg)
{
  ft_debug("[ftc] accept return");
  struct ft_compiler* c = (struct ft_compiler*)arg;
  struct ft_bytecode_builder* bb = c->context;
  int contexts = ft_compiler_explicit_return_context_count(c);
  c->error_code = ft_bb_x_return(bb, c->env, contexts);
  ft_debug("  return contexts= %d", contexts);
}

void ftc_accept_statement(void* arg)
{
  ft_debug("[ftc] accept statement");
  struct ft_compiler* c = (struct ft_compiler*)arg;
  struct ft_bytecode_builder* bb = c->context;
  switch(ft_bb_context_type(bb))
  {
  case ftbbf_method_context:
  case ftbbf_closure_context:
    if(bb->stack_size)
    {
      c->error_code = ft_bb_x_pop(bb, c->env, bb->stack_size);
    }
    break;

  case ftbbf_object_context:
  {
    int flags = ft_bb_get_flags(c->context);
    if(flags & ftbbf_last_statement_was_method)
    {
      ft_bb_set_flags(c->context, flags & ~ftbbf_last_statement_was_method);
      return;
    }

    // expect the stack size to be equivalent to the variable count
    int slot_count = ft_bb_slot_count(bb);
    if(bb->stack_size != slot_count)
    {
      ft_debug("throwing object stack error stack size = %d  var count = %d", bb->stack_size, slot_count);
      c->error_code = ftbbec_object_stack_error;
      return;
    }
    break;
  }
  }
}



void ft_compiler_push_object_context(struct ft_compiler* co)
{
  struct ft_bytecode_builder* old_context = co->context++;
  ft_init_bytecode_builder_object_context(co->context, co->env, old_context, old_context->slots_end);
}

void ft_compiler_pop_object_context(struct ft_compiler* co)
{
  struct ft_bytecode_builder* object_context = co->context--;
  co->context->ip = object_context->ip;
  co->context->stack_size += object_context->stack_size;
}

void ftc_begin_object(void* arg)
{
  ft_debug("[ftc] begin object");
  struct ft_compiler* c = (struct ft_compiler*)arg;
  ft_compiler_push_object_context(c);
}

void ftc_accept_object(void* arg)
{
  ft_debug("[ftc] accept object");
  struct ft_compiler* c = (struct ft_compiler*)arg;
  ft_bb_finish_object_context(c->context, c->env, c->options->vtable_vt);
  ft_compiler_pop_object_context(c);
}




void ftc_begin_method(const char* selector, int argc, const char** argv, void* arg)
{
  ft_debug("[ftc] begin method '%s'\n", selector);
  struct ft_compiler* c = (struct ft_compiler*)arg;

  assert(argc); // method must at least have a self parameter

  struct ft_bytecode_builder* old_context = c->context,
    *new_context = ++c->context;
  ft_init_bytecode_builder(new_context, c->env, ft_deep_clone(ft_environment_allocator(c->env), c->options->bytecode_prototype, 2), ftbbf_method_context, old_context->slots_end);
  for(int i = 0; i < argc; ++i)
  {
    bool is_parent = i == 0;
    ft_bb_add_variable(new_context, c->env, argv[i], 0, is_parent);
  }
}

void ftc_accept_method(const char* selector, int argc, const char** argv, void* arg)
{
  ft_debug("[ftc] accept method '%s'\n", selector);
  struct ft_compiler* c = (struct ft_compiler*)arg;
  struct ft_bytecode_builder* method_builder = c->context--;
  oop method = ft_bb_create_method(method_builder, c->env, c->options->vtable_vt);

  // ft_bb_add_variable(c->context, c->env, selector, ftcvt_method, method);
  ft_bb_add_slot(c->context, c->env, selector, ftvs_static_slot, method);

  ft_bb_set_flags(c->context, ft_bb_get_flags(c->context) | ftbbf_last_statement_was_method);
}



void ftc_begin_block(int argc, const char** argv, void* arg)
{
  ft_debug("[ftc] begin block argc= %d", argc);
  struct ft_compiler* c = (struct ft_compiler*)arg;

  struct ft_bytecode_builder* old_context = c->context,
    *new_context = ++c->context;

  oop bytecode = ft_deep_clone(ft_environment_allocator(c->env), c->options->bytecode_prototype, 2);
  ft_init_bytecode_builder(new_context, c->env, bytecode, ftbbf_closure_context, old_context->slots_end);
  ft_bb_add_variable(new_context, c->env, "self", 0, 1); // ftcvt_parent, NULL);
  for(int i = 0; i < argc; ++i)
  {
    ft_bb_add_variable(new_context, c->env, argv[i], 0, 0); // ftcvt_local, NULL);
  }

  ft_debug("  old context= '%p' type= '%d'  new context= '%p' type= '%d'",
    old_context,
    ft_bb_context_type(old_context),
    new_context,
    ft_bb_context_type(new_context));
}

void ftc_accept_block(int argc, const char** argv, void* arg)
{
  ft_debug("[ftc] accept block");
  struct ft_compiler* c = (struct ft_compiler*)arg;
  struct ft_bytecode_builder* method_builder = c->context--;
  oop method = ft_bb_create_method(method_builder, c->env, c->options->vtable_vt);

  char *selector_buffer = calloc(1 + (argc ? argc : 1) * 6, 1);
  // TODO check for null selector_buffer
  if(argc)
  {
    char* selector_end = selector_buffer;
    for(int i = argc; i--;)
    {
      memcpy(selector_end, "value:", 6);
      selector_end += 6;
      *selector_end++ = 0;
    }
  }
  else
  {
    memcpy(selector_buffer, "value", 5);
  }
  struct ft_string* value_selector = ft_environment_intern(c->env, selector_buffer);
  free(selector_buffer);

  // create a vtable for the block
  struct ft_vtable_slot_desc block_slots[2];
  ft_vtable_slot_desc_init(block_slots+0, ftvs_parent_slot, ft_environment_intern(c->env, "lexicalParent"), NULL);
  ft_vtable_slot_desc_init(block_slots+1, ftvs_static_slot, value_selector, method);

  struct ft_vtable* block_vt = ft_new_vtable(ft_environment_allocator(c->env), c->options->vtable_vt,
    2, block_slots, 0);

  ft_bb_x_push_context(c->context, c->env);
  ft_bb_x_literal(c->context, c->env, block_vt);
  ft_bb_x_send(c->context, c->env, 1, "__VTable_instantiate");
  ft_bb_x_init_slot(c->context, c->env, 0);
}


struct ft_parser_actions ftpa_compiler_vt = {
  ftc_should_continue,
  ftc_accept_send,
  ftc_accept_self_send,
  ftc_accept_integer,
  ftc_accept_string,
  ftc_accept_assignment,
  ftc_accept_identifier,
  ftc_accept_return,
  ftc_accept_statement,
  ftc_begin_object,
  ftc_accept_object,
  ftc_begin_method,
  ftc_accept_method,
  ftc_begin_block,
  ftc_accept_block
};





void ft_init_compiler(struct ft_compiler* co, struct ft_environment* env, struct ft_compiler_options* opt)
{
  ft_debug("init top level compiler %p", co);
  memset(co, 0, sizeof(struct ft_compiler));
  co->env = env;
  co->options = opt;
  co->context = co->context_stack;
  ft_init_bytecode_builder(co->context, co->env, ft_deep_clone(ft_environment_allocator(co->env), opt->bytecode_prototype, 2), ftbbf_method_context, co->slot_buffer);
  ft_bb_add_variable(co->context, co->env, "lobby", 0, 1); // ftcvt_parent, NULL);
}




// void ft_init_method_compiler(struct ft_compiler* co, oop bytecode_object)
// {

// }

// void ft_init_compiler(struct ft_compiler* co, struct ft_environment* env, )


const char* vm_opcode_names[] = {
  "literal", "send", "self_send", "return",
  "init_slot", "pop", "push_context", "extend"
};

void ft_debug_print_bytecode(ft_object bytecode)
{
#if !FT_DEBUG_COMPILER
  return;
#endif

  struct ft_bytes* instructions = (struct ft_bytes*)bytecode[0];
  ft_instruction_t* ip = ft_bytes_data_begin(instructions);
  fprintf(stderr, "bytecode\n");
  for(int x = 0; x < 100; ++x)
  {
    ft_instruction_t op = *ip++;
    enum ft_vm_instruction instruction = op & ftvmi__mask;
    int arg = op >> ftvmi__bits;
    fprintf(stderr, " %d. %s %d\n", x, vm_opcode_names[instruction], arg);
    if(instruction == ftvmi_return)
    {
      break;
    }
  }
  struct ft_array* literals = (struct ft_array*)bytecode[1];
  fprintf(stderr, "literals\n");
  for(oop* iter = ft_array_items_begin(literals), *end = ft_array_items_end(literals);
    iter != end;
    ++iter)
  {
    oop value = *iter;
    if(OOP_IS_INT(value))
      fprintf(stderr, " %ld\n", OOP_INT(value));
    else if(!value)
      fprintf(stderr, " nil\n");
    else
      fprintf(stderr, " '%s'\n", ft_string_bytes((struct ft_string*)value));
  }
}



void ft_compiler_describe_error(struct ft_compiler* co, int size, char* output)
{
  switch(co->error_code)
  {
  case ftbbec_stack_underflow:
    snprintf(output, size, "stack underflow");
    return;
  case ftbbec_object_stack_error:
    snprintf(output, size, "object stack error");
    return;
  case ftbbec_none:
    snprintf(output, size, "none");
    return;
  }
}


bool ft_compiler_has_error(struct ft_compiler* co)
{
  return (bool)co->error_code;
}


oop ft_compile(struct ft_environment* env, struct ft_compiler_options* compiler_options, struct ft_parser_input* input)
{
  struct ft_parser parser;
  ft_init_parser(&parser, &ftpa_compiler_vt, input);

  struct ft_compiler compiler;
  ft_init_compiler(&compiler, env, compiler_options);

  ft_parser_parse_script(&parser, (void*)&compiler);
  if(parser.error_code)
  {
    fprintf(stderr, "parser error '%d'\n%s\n", (int)parser.error_code, parser.error_message);
    exit(21);
    return OOP_NIL;
  }

  if(ft_compiler_has_error(&compiler))
  {
    char str[256];
    ft_compiler_describe_error(&compiler, sizeof(str), str);
    fprintf(stderr, "compiler error '%d'\n%s\n", (int)compiler.error_code, str);
    exit(22);
    return OOP_NIL;
  }

  oop method = ft_bb_create_method(compiler.context, env, compiler_options->vtable_vt);

  ft_debug_print_bytecode(compiler.context->bytecode);

  return method;
}

oop ft_compile_file(struct ft_environment* env, struct ft_compiler_options* compiler_options, FILE* file)
{
  struct ft_parser_file_input input;
  ft_init_parser_file_input(&input, file);

  return ft_compile(env, compiler_options, (struct ft_parser_input*)&input);
}

oop ft_compile_string(struct ft_environment* env, struct ft_compiler_options* compiler_options, int size, const char* string)
{
  struct ft_parser_string_input input;
  ft_init_parser_string_input(&input, size, string);

  return ft_compile(env, compiler_options, (struct ft_parser_input*)&input);
}


// initialize compiler options from an environment
void ft_init_compiler_options(struct ft_environment* env, struct ft_compiler_options* opt)
{
  opt->bytecode_prototype = ft_simple_lookup(ft_environment_lobby(env), ft_environment_intern(env, "BytecodePrototype"));
  opt->vtable_vt = ft_simple_lookup(ft_environment_lobby(env), ft_environment_intern(env, "VTable"));
}




