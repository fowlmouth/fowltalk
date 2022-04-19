#ifndef FT_DEBUG_VM
#define FT_DEBUG_VM 0
#endif

#define FT_DEBUG FT_DEBUG_VM
#include "fowltalk/debug.h"

#include "fowltalk.h"
#include "fowltalk/vm.h"
#include "fowltalk/bytes.h"
#include "fowltalk/environment.h"
#include "fowltalk/vtable.h"
#include "fowltalk/array.h"
#include "fowltalk/string.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>


struct ft_virtual_machine_frame
{
  oop* sp;
  ft_instruction_t* ip;
  ft_object method;
};

struct ft_virtual_machine
{
  enum ft_vm_error_code error_code;
  struct ft_virtual_machine_frame *frame;

  struct ft_virtual_machine_frame callstack[FT_VM_CALLSTACK_SIZE];
  oop stack[FT_VM_STACK_SIZE];
};

ft_instruction_t* method_bytecode_iseq(ft_object object)
{
  struct ft_vtable* vt = ft_object_vtable(object);
  oop bytecode = ft_vtable_get_code(vt);
  oop instructions = ((ft_object)bytecode)[0];
  return ft_bytes_data_begin(instructions);
}

void init_vm(struct ft_virtual_machine* vm, oop method)
{
  memset(vm, 0, sizeof(struct ft_virtual_machine));
  struct ft_virtual_machine_frame* f = vm->frame = vm->callstack;
  f->sp = vm->stack;
  f->method = (ft_object)method;
  f->ip = method_bytecode_iseq(f->method);
}

struct ft_virtual_machine* ft_new_vm(struct ft_allocator* allocator, struct ft_vtable* vtable, oop method)
{
  struct ft_virtual_machine* vm = (struct ft_virtual_machine*)ft_allocator_alloc(allocator, vtable, sizeof(struct ft_virtual_machine));
  init_vm(vm, method);
  return vm;
}

int ft_vm_struct_size()
{
  return sizeof(struct ft_virtual_machine);
}

void ft_vm_set_error(struct ft_virtual_machine* vm, enum ft_vm_error_code error_code)
{
  vm->error_code = error_code;
}

enum ft_vm_error_code ft_vm_error_code(struct ft_virtual_machine* vm)
{
  return vm->error_code;
}


int ft_vm_describe_error(struct ft_virtual_machine* vm, char* output, int size)
{
  const char* error_string = NULL;
  switch(ft_vm_error_code(vm))
  {
  case ftvmec_none:
    error_string = "none";
    break;

  case ftvmec_complete:
    error_string = "complete";
    break;

  case ftvmec_stack_underflow:
    error_string = "stack underflow";
    break;

  case ftvmec_stack_overflow:
    error_string = "stack overflow";
    break;

  case ftvmec_callstack_underflow:
    error_string = "call stack underflow";
    break;

  case ftvmec_callstack_overflow:
    error_string = "call stack overflow";
    break;

  case ftvmec_invalid_instruction:
    error_string = "invalid instruction";
    break;

  case ftvmec_invalid_primitive:
    error_string = "invalid primitive";
    break;

  case ftvmec_invalid_return:
    error_string = "invalid non-local return";
    break;

  case ftvmec_object_stack_error:
    error_string = "object stack error";
    break;

  case ftvmec_invalid_unknown_message_handler:
    error_string = "invalid unknownMessage:withArguments: slot";
    break;
  }

  if(error_string)
  {
    int len = strlen(error_string);
    int copy_len = len < (size-1) ? len : (size-1);
    strncpy(output, error_string, copy_len);
    output[copy_len] = 0;
    return 1;
  }

  snprintf(output, size, "unknown '%d'", (int)ft_vm_error_code(vm));
  return 0;
}




struct ft_virtual_machine_frame* ft_vm_callstack_end(struct ft_virtual_machine* vm)
{
  return vm->callstack + FT_VM_CALLSTACK_SIZE;
}

bool ft_vm_push_frame(struct ft_virtual_machine* vm, oop method)
{
  ft_debug("method= '%p'", method);
  struct ft_virtual_machine_frame* old_frame = vm->frame;
  struct ft_virtual_machine_frame* new_frame = ++vm->frame;
  if(new_frame >= ft_vm_callstack_end(vm))
  {
    vm->error_code = ftvmec_callstack_overflow;
    return false;
  }
  new_frame->sp = old_frame->sp;
  new_frame->method = (ft_object)method;
  new_frame->ip = method_bytecode_iseq(new_frame->method);
  return true;
}

void ft_vm_push(struct ft_virtual_machine* vm, oop value)
{
  *vm->frame->sp++ = value;
}

oop ft_vm_pop(struct ft_virtual_machine* vm, int count)
{
  oop result = OOP_NIL;
  for(; count--;)
    result = *--vm->frame->sp;
  return result;
}

oop ft_vm_quick_activate_and_run(struct ft_vm_quick_send_options* opt, oop method, oop default_value, void(*handle_error)(struct ft_virtual_machine*))
{
  struct ft_virtual_machine vm;
  init_vm(&vm, method);
  enum ft_vm_run_result result = ft_vm_run(&vm, opt->env, opt->ticks);

  if(result == ftvmr_complete)
  {
    return vm.stack[0];
  }
  else if(result == ftvmr_error)
  {
    if(handle_error)
    {
      handle_error(&vm);
    }
  }
  return default_value;
}

oop ft_vm_quick_send(struct ft_vm_quick_send_options* opt, oop receiver, struct ft_string* selector, int argc, oop* argv)
{
  oop context = OOP_NIL, method = OOP_NIL;
  ft_integer_t setter_index = -1;
  enum ft_lookup_result lookup_result = ft_lookup(receiver, selector, &context, &method, &setter_index);
  if(lookup_result == ftlr_not_found)
    return OOP_NIL;
  if(lookup_result == ftlr_setter)
  {
    if(argc > 1)
    {
      // set the slot value, return receiver
      ((ft_object)context)[setter_index] = argv[1];
      return argv[0];
    }
    return OOP_NIL;
  }

  struct ft_vtable* vt = ft_object_vtable(method);
  oop code = ft_vtable_get_code(vt);
  if(code)
  {
    // fill in the args and activate the method
    method = ft_clone(ft_environment_allocator(opt->env), method);
    struct ft_vtable* method_vt = ft_object_vtable(method);
    oop* iter = ft_vtable_parents_end(method_vt, method);
    oop* end = ft_vtable_data_slots_end(method_vt, method);
    if(end > iter + argc)
    {
      end = iter + argc;
    }
    for(; iter < end; ++iter)
    {
      *iter++ = *argv++;
    }

    return ft_vm_quick_activate_and_run(opt, method, OOP_NIL, NULL);
  }
  return method;
}


bool ft_is_activatable_method(oop method)
{
  struct ft_vtable* vt = ft_object_vtable(method);
  return (bool)ft_vtable_get_code(vt);
}

bool ft_prepare_activatable_method(struct ft_allocator* allocator, oop* method, int argc, oop* argv)
{
  if(OOP_IS_PTR(*method) && ft_is_activatable_method(*method))
  {
    *method = ft_clone(allocator, *method);
    // copy args
    // argv 0 goes to parent 0 the rest start at data slot 0
    struct ft_vtable* method_vt = ft_object_vtable(*method);
    oop* parents = ft_vtable_parents_begin(method_vt, *method);
    oop* data_slots = ft_vtable_parents_end(method_vt, *method);
    parents[0] = *argv++;
    --argc;
    for(; argc--;)
    {
      *data_slots++ = *argv++;
    }
    return true;
  }
  return false;
}

bool ft_selector_is_primitive(struct ft_string* selector)
{
  const char* bytes = ft_string_bytes(selector);
  return ft_string_size(selector) > 2 && bytes[0] == '_' && bytes[1] == '_';
}

void ft_vm_send(struct ft_virtual_machine* vm, struct ft_environment* env, struct ft_string* selector, int argc, oop* argv)
{
  const char* bytes = ft_string_bytes(selector);
  ft_debug("vm send '%s' argc= %d", bytes, argc);
  if(ft_selector_is_primitive(selector))
  {
    // primitive method!
    ft_primitive_t prim = ft_environment_primitive(env, selector);
    if(!prim)
    {
      fprintf(stderr, "invalid primitive '%s'\n", ft_string_bytes(selector));
      ft_vm_set_error(vm, ftvmec_invalid_primitive);
      return;
    }
    prim(vm, env, argc, argv);
  }
  else
  {
    oop context = OOP_NIL, method = OOP_NIL;
    ft_integer_t setter_index = -1;
    enum ft_lookup_result lookup_result = ft_lookup(*argv, selector, &context, &method, &setter_index);
    ft_debug("  lookup result= '%d'", (int)lookup_result);
    switch(lookup_result)
    {
    case ftlr_not_found:
    {
      ft_debug("ftlr not found");
      ft_vtable_debug_print(ft_object_vtable(argv[0]));
      // create a Message object and send it to the receiver with unknownMessage:
      lookup_result = ft_lookup(*argv, ft_environment_intern(env, "unknownMessage:withArguments:"), &context, &method, &setter_index);
      if(lookup_result != ftlr_data)
      {
        // throw error
        ft_vm_set_error(vm, ftvmec_invalid_unknown_message_handler);
        return;
      }

      if(OOP_IS_PTR(method) && ft_is_activatable_method(method))
      {
        oop arguments = OOP_NIL;
        if(argc > 1)
        {
          struct ft_vtable* array_vt = ft_object_vtable(ft_simple_lookup(ft_environment_lobby(env), ft_environment_intern(env, "Array")));
          arguments = ft_new_array(ft_environment_allocator(env), array_vt, argc-1);
          for(int i = 1; i < argc; ++i)
          {
            ft_array_items_begin(arguments)[i-1] = argv[i];
          }
        }
        oop argv2[3] = {argv[0], selector, arguments};
        if(ft_prepare_activatable_method(ft_environment_allocator(env), &method, 3, argv2))
        {
          ft_debug("  activating unknownMessage:withArguments: method");
          ft_vm_push_frame(vm, method);
        }
        else
        {
          ft_debug("  unknownMessage:withArguments: was not activatable");
          ft_vm_push(vm, method);
        }
      }
      else
      {
        // TODO is this the right behavior?
        // here the unknownMethod: lookup returned an object which is *not* a method
        // for now we will just push that object onto the stack
        ft_vm_push(vm, method);
      }
      break;
    }

    case ftlr_data:
      if(ft_prepare_activatable_method(ft_environment_allocator(env), &method, argc, argv))
      {
        ft_debug("  activating method");
        ft_vm_push_frame(vm, method);
      }
      else
      {
        ft_debug("  not activatable");
        ft_vm_push(vm, method);
      }
      break;

    case ftlr_setter:
      ft_debug("ftlr setter index= %ld", setter_index);
      ((ft_object)context)[setter_index] = argv[1];
      ft_vm_push(vm, *argv);
      break;

    }
  }
}

void ft_vm_send_from_stack(struct ft_virtual_machine* vm, struct ft_environment* env, struct ft_string* selector, int argc)
{
  ft_vm_pop(vm, argc);
  ft_vm_send(vm, env, selector, argc, vm->frame->sp);
}

void ft_vm_debug_print_stack(struct ft_virtual_machine* vm)
{
  char buffer[1024];
  char* buffer_end = buffer + sizeof(buffer);
  char* str = buffer;
  const char* separator = "";
  oop* end_sp = vm->frame->sp;
  str += snprintf(str, buffer_end - str, "[ ");
  for(oop* sp = vm->stack; sp < end_sp; ++sp)
  {
    int result = snprintf(str, buffer_end - str, "%s%p", separator, *sp);
    str += result;
    separator = ", ";
  }
  str += snprintf(str, buffer_end - str, " ]");
  fprintf(stderr, "stack: %s\n", buffer);
}

void ft_vm_debug_print_literals(struct ft_virtual_machine* vm)
{
  ft_object bytecode = ft_vtable_get_code(ft_object_vtable(vm->frame->method));
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

void ft_vm_debug_print_object(oop object)
{
  char buffer[1024];
  char* buffer_end = buffer + sizeof(buffer);
  char* str = buffer;
  const char* separator = "";
  str += snprintf(str, buffer_end - str, "{ ");

  struct ft_vtable* vt = ft_object_vtable(object);
  for(struct ft_vtable_slot* iter = ft_vtable_slots_begin(vt), *end = ft_vtable_slots_end(vt);
    iter != end;
    iter = ft_vtable_slot_next(iter))
  {
    if(!ft_vtable_slot_name(iter))
      continue;

    str += snprintf(str, buffer_end - str, "%s", separator);
    oop value;
    switch(ft_vtable_slot_type(iter))
    {
    case ftvs_data_slot:
      value = ((ft_object)object)[OOP_INT(ft_vtable_slot_value(iter))];
      str += snprintf(str, buffer_end - str, "%s = %p", ft_string_bytes(ft_vtable_slot_name(iter)), value);
      break;

    case ftvs_parent_slot:
      value = ((ft_object)object)[OOP_INT(ft_vtable_slot_value(iter))];
      str += snprintf(str, buffer_end - str, "%s* = %p", ft_string_bytes(ft_vtable_slot_name(iter)), value);
      break;

    case ftvs_setter_slot:
      value = ft_vtable_slot_value(iter);
      str += snprintf(str, buffer_end - str, "%s = setter(%ld)", ft_string_bytes(ft_vtable_slot_name(iter)), OOP_INT(value));
      break;

    case ftvs_static_slot:
      str += snprintf(str, buffer_end - str, "%s = static(%p)", ft_string_bytes(ft_vtable_slot_name(iter)), ft_vtable_slot_value(iter));
      break;

    case ftvs_static_parent:
      value = ft_vtable_static_parents_begin(vt)[ OOP_INT(ft_vtable_slot_value(iter)) ];
      str += snprintf(str, buffer_end - str, "%s* = static(%p)", ft_string_bytes(ft_vtable_slot_name(iter)), value);
      break;
    }

    separator = ". ";
  }
  str += snprintf(str, buffer_end - str, " }");
  fprintf(stderr, "%s\n", buffer);
}

void ft_vm_debug_print_context(struct ft_virtual_machine* vm)
{
  fprintf(stderr, "context: ");
  ft_vm_debug_print_object(vm->frame->method);
}

void ft_vm_debug_print_all(struct ft_virtual_machine* vm)
{
#if !FT_DEBUG_VM
  return;
#endif
  ft_vm_debug_print_stack(vm);
  ft_vm_debug_print_context(vm);
  fprintf(stderr, "\n");
}

void ft_vm_pop_into(struct ft_virtual_machine* vm, int count, oop* destination)
{
  int n = count;
  for(oop* src = vm->frame->sp - count; n--;)
  {
    *destination++ = *src++;
  }
  vm->frame->sp -= count;
}

void ft_vm_finish(struct ft_virtual_machine* vm, oop value)
{
  vm->error_code = ftvmec_complete;
  vm->stack[0] = value;
}

enum ft_vm_run_result ft_vm_run(struct ft_virtual_machine* vm, struct ft_environment* env, int ticks)
{
  if(vm->error_code)
    return ftvmr_error;

  ft_object bytecode, literals;
  struct ft_string* selector = NULL;
  unsigned long arg = 0;
  ft_instruction_t instruction = 0, op = 0;

  const ft_instruction_t arg_bits = CHAR_BIT - ftvmi__bits;
  const ft_instruction_t arg_mask = (1 << arg_bits) - 1;

  const int send_argc_bits = 5;

#define UPDATE_CONTEXT \
    do{ \
      bytecode = (ft_object)ft_vtable_get_code(ft_object_vtable(vm->frame->method)); \
      literals = ft_array_items_begin(bytecode[1]); \
      ft_debug(" new context= %p  frame id= %ld  ip= %ld", \
        vm->frame->method, \
        vm->frame - vm->callstack, \
        vm->frame->ip - method_bytecode_iseq(vm->frame->method)); \
    }while(0)

  UPDATE_CONTEXT;

  while(!vm->error_code && (ticks > 0 || op == ftvmi_extend))
  {
#if FT_DEBUG_VM
    int debug_ip = vm->frame->ip - ft_bytes_data_begin(bytecode[0]);
    int debug_ip_high = ft_bytes_size(bytecode[0]);
#endif
    instruction = *vm->frame->ip++;
    op = instruction & ftvmi__mask;
    arg <<= arg_bits;
    arg |= (instruction >> ftvmi__bits) & arg_mask;

    ft_debug(" ip= %d / %d  op= %d  arg= %lu  context= %p",
      debug_ip, debug_ip_high, (int)op, arg,
      vm->frame->method);

    switch(op)
    {
    case ftvmi_literal:
      ft_debug("literal index= %lu", arg);
      ft_vm_push(vm, literals[arg]);
      arg = 0;
      break;

    case ftvmi_send:
      // argc = arg & ((1 << send_argc_bits) - 1);
      // arg >>= send_argc_bits; // arg is now the index of the selector in literals
      selector = (struct ft_string*)ft_vm_pop(vm, 1);
      ft_debug("send argc= %lu selector= '%s'", arg, ft_string_bytes(selector) );
      ft_vm_send_from_stack(vm, env, selector, arg);
      arg = 0;
      UPDATE_CONTEXT;
      break;

    case ftvmi_self_send:
    {
      // argc = arg & ((1 << send_argc_bits) - 1);
      // arg >>= send_argc_bits; // arg is now the index of the selector in literals
      selector = (struct ft_string*)ft_vm_pop(vm, 1);
      ft_debug("self send argc= %lu selector= '%s'", arg, ft_string_bytes(selector));
      oop argv[32];
      argv[0] = vm->frame->method;
      ft_vm_pop_into(vm, arg, argv+1);
      ft_vm_send(vm, env, selector, arg+1, argv);
      arg = 0;
      UPDATE_CONTEXT;
      break;
    }

    case ftvmi_init_slot:
    {
      ft_debug("init slot index= %lu", arg);
      oop target = ft_vm_pop(vm, 1);
      oop value = ft_vm_pop(vm, 1);
      ((ft_object)target)[arg] = value;
      ft_vm_push(vm, target);
      arg = 0;
#if FT_DEBUG_VM
      fprintf(stderr, "init'd object: %p ", target);
      ft_vm_debug_print_object(target);
      ft_vtable_debug_print(ft_object_vtable(target));
#endif
      break;
    }

    case ftvmi_return:
    {
      ft_debug("return contexts= %lu", arg);
      oop context = vm->frame->method;
      for(int c = arg; c--;)
      {
        struct ft_vtable* context_vt = ft_object_vtable(context);
        ft_object context_parents = ft_vtable_parents_begin(context_vt, context);
        context = context_parents[0]; // "self"
        context_vt = ft_object_vtable(context);
        context_parents = ft_vtable_parents_begin(context_vt, context);
        context = context_parents[0]; // "lexicalParent"
        // context = ((ft_object)parents[0])[0];
      }
      // make sure context is on the callstack
      struct ft_virtual_machine_frame* frame;
      for(frame = vm->frame;
        frame >= vm->callstack;
        --frame)
      {
        if(frame->method == context)
        {
          break;
        }
      }
      if(frame < vm->callstack)
      {
        // context wasn't found on the callstack
        vm->error_code = ftvmec_invalid_return;
        return ftvmr_error;
      }
      --frame; // we will return to this frame's caller

      oop value = ft_vm_pop(vm, 1);
      if(frame < vm->callstack)
      {
        // if frame underflows the vm has nothing left to run
        ft_vm_finish(vm, value);
        return ftvmr_complete;
      }
      vm->frame = frame;
      ft_vm_push(vm, value);
      arg = 0;
      UPDATE_CONTEXT;
      break;
    }

    case ftvmi_pop:
      ft_debug("pop %lu", arg);
      ft_vm_pop(vm, arg);
      arg = 0;
      break;

    case ftvmi_push_context:
      ft_debug("push context");
      ft_vm_push(vm, vm->frame->method);
      arg = 0;
      break;

    case ftvmi_extend:
      ft_debug("extend value= %lu", arg);
      break;

    default:
      ft_debug("error: invalid instruction %d", (int)op);
      vm->error_code = ftvmec_invalid_instruction;
      return ftvmr_error;
    }

    ft_vm_debug_print_all(vm);

  }


  switch(vm->error_code)
  {
  case ftvmec_none:
    return ftvmr_ok;
  case ftvmec_complete:
    return ftvmr_complete;
  default:
    return ftvmr_error;
  }

#undef UPDATE_CONTEXT
}


