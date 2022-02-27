#ifndef H_FOWLTALK_VM
#define H_FOWLTALK_VM

#include "fowltalk.h"
#include <stdint.h>

#define FT_VM_CALLSTACK_SIZE 128
#define FT_VM_STACK_SIZE 256

enum ft_vm_instruction
{
  ftvmi_push_context, ftvmi_literal,
  ftvmi_send, ftvmi_self_send,
  ftvmi_return, ftvmi_init_slot,
  ftvmi_pop, ftvmi_extend,

  ftvmi__bits = 3,
  ftvmi__mask = (1 << ftvmi__bits) - 1
};
typedef uint8_t ft_instruction_t;

enum ft_vm_run_result
{
  ftvmr_complete, ftvmr_ok, ftvmr_error
};

enum ft_vm_error_code
{
  ftvmec_none = 0,
  ftvmec_complete = 0x10,
  ftvmec_stack_underflow = 0x100,
  ftvmec_stack_overflow,
  ftvmec_callstack_underflow,
  ftvmec_callstack_overflow,
  ftvmec_invalid_instruction,
  ftvmec_invalid_primitive,
  ftvmec_invalid_return,
  ftvmec_object_stack_error,
  ftvmec_invalid_unknown_message_handler
};


struct ft_vm_quick_send_options
{
  struct ft_environment* env;
  struct ft_vtable* vm_vt;
  int ticks;
};

struct ft_virtual_machine;

struct ft_virtual_machine* ft_new_vm(struct ft_allocator* allocator, struct ft_vtable* vtable, oop method);
int ft_vm_describe_error(struct ft_virtual_machine* vm, char* output, int size);
enum ft_vm_run_result ft_vm_run(struct ft_virtual_machine* vm, struct ft_environment* env, int ticks);
oop ft_vm_quick_activate_and_run(struct ft_vm_quick_send_options* opt, oop method, oop default_value, void(*handle_error)(struct ft_virtual_machine*));
int ft_vm_struct_size();

void ft_vm_push(struct ft_virtual_machine*, oop);
bool ft_vm_push_frame(struct ft_virtual_machine* vm, oop method);

enum ft_vm_error_code ft_vm_error_code(struct ft_virtual_machine*);

#endif
