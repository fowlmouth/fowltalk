#include "fowltalk/primitives.h"
#include "fowltalk/environment.h"
#include "fowltalk/vtable.h"
#include "fowltalk/string.h"
#include "fowltalk/vm.h"
#include "fowltalk/compiler.h"

#include <string.h>

#include <stdio.h>


#define PRIM(name) \
  void ft_primitive_##name(struct ft_virtual_machine* vm, struct ft_environment* env, int argc, oop* argv)

PRIM(__Lobby)
{
  ft_vm_push(vm, ft_environment_lobby(env));
}

void print_slot(enum ft_vtable_slot_type type, struct ft_string* name, oop value)
{
  const char *str_type = "";
  switch(type)
  {
  case ftvs_parent_slot:
    str_type = "parent";
    break;
  case ftvs_static_parent:
    str_type = "static parent";
    break;
  case ftvs_static_slot:
    str_type = "static";
    break;
  case ftvs_data_slot:
    str_type = "data";
    break;
  case ftvs_setter_slot:
    str_type = "setter";
    break;
  }
  printf("  %s %s(%ld)\n", ft_string_bytes(name), str_type, OOP_INT(value));
}

PRIM(__Object_printSlots)
{
  struct ft_vtable* vt = ft_object_vtable(argv[0]);
  ft_vtable_print_stats(vt);
  printf("\n");
  ft_vtable_each_slot(vt, print_slot);
  ft_vm_push(vm, argv[0]);
}

PRIM(__Object_respondsTo_)
{
  ft_integer_t setter_index;
  oop context, result;
  enum ft_lookup_result lookup_result = ft_lookup(argv[0], 
    (struct ft_string*)argv[1],
    &context,
    &result,
    &setter_index);
  if(lookup_result == ftlr_not_found)
  {
    ft_vm_push(vm, INT_OOP(0));
  }
  else
  {
    ft_vm_push(vm, INT_OOP(1));
  }

}

PRIM(__Object_copyAddingSlot_value_)
{

}
PRIM(__Object_addStaticSlot_value_)
{
  struct ft_vtable* vtable = ft_object_vtable(argv[0]);
  struct ft_string* selector = (struct ft_string*)argv[1];
  struct ft_vtable_slot* vt_slot = ft_vtable_lookup(vtable, selector);
  if(vt_slot)
  {
    fprintf(stderr, "__Object_addStaticSlot:value: - slot '%s' already exists\n", 
      ft_string_bytes(ft_vtable_slot_name(vt_slot)));
    ft_vm_push(vm, argv[0]);
    return;
  }
  int vt_capacity = ft_vtable_slot_capacity(vtable);
  struct ft_vtable_slot_desc new_slots[vt_capacity+2];
  int slot_count = ft_vtable_decompose_slots(vtable, vt_capacity+2, new_slots);
  ft_vtable_slot_desc_init(new_slots + slot_count++,
    ftvs_static_slot, selector, argv[2]);
  for(int i = 0 ; i < slot_count ; ++i)
  {
    printf("  %s '%d'\n", ft_string_bytes(new_slots[i].name), new_slots[i].type);
  }
  struct ft_vtable* new_vtable = ft_new_vtable(ft_environment_allocator(env),
    ft_object_vtable(vtable), slot_count, new_slots, ft_vtable_base_size(vtable));
  ft_set_vtable(argv[0], new_vtable);
  ft_vm_push(vm, argv[0]);
}
PRIM(__Object_setSlot_)
{

}

PRIM(__String_concat_)
{
  struct ft_string* a = (struct ft_string*)argv[0],
    *b = (struct ft_string*)argv[1];
  int len_a = ft_string_size(a),
    len_b = ft_string_size(b);
  struct ft_string* c = ft_new_string(ft_environment_allocator(env),
    ft_object_vtable(argv[0]), len_a + len_b, NULL);
  const char* str_a = ft_string_bytes(a), *str_b = ft_string_bytes(b);
  memcpy(ft_string_bytes_begin(c), str_a, len_a);
  memcpy(ft_string_bytes_begin(c)+len_a, str_b, len_b);
  *ft_string_bytes_end(c) = 0;
  // printf("concat '%s' with '%s' equals '%s'", str_a, str_b, ft_string_bytes(c));
  ft_vm_push(vm, c);
}
PRIM(__String_readFile)
{
  // fprintf(stderr, "__String_readFile :: %s\n", ft_string_bytes((struct ft_string*)argv[0]));
  struct ft_string* receiver = (struct ft_string*)argv[0];
  FILE* f = fopen(ft_string_bytes(receiver), "r");
  fseek(f, 0, SEEK_END);
  int size = ftell(f);
  fseek(f, 0, SEEK_SET);
  char* buffer = malloc(size);
  fread(buffer, 1, size, f);
  fclose(f);
  struct ft_string* contents = ft_new_string(ft_environment_allocator(env),
    ft_object_vtable(receiver),
    size,
    buffer);
  free(buffer);
  ft_vm_push(vm, contents);
}
PRIM(__String_compileAnonymousMethod)
{
  fprintf(stderr, "__String_compileAnonymousMethod :: %s\n", ft_string_bytes((struct ft_string*)argv[0]));

  struct ft_compiler_options compiler_options;
  ft_init_compiler_options(env, &compiler_options);
  oop method = ft_compile_string(env, &compiler_options,
    ft_string_size((struct ft_string*)argv[0]),
    ft_string_bytes((struct ft_string*)argv[0]));

  ft_vm_push(vm, method);
}

PRIM(__String_print)
{
  printf("%s", ft_string_bytes(argv[0]));
  ft_vm_push(vm, argv[0]);
}
PRIM(__String_printLn)
{
  printf("%s\n", ft_string_bytes(argv[0]));
  ft_vm_push(vm, argv[0]);
}

PRIM(__Integer_add_)
{
  ft_vm_push(vm, INT_OOP(OOP_INT(argv[0]) + OOP_INT(argv[1])));
}
PRIM(__Integer_subtract_)
{
  ft_vm_push(vm, INT_OOP(OOP_INT(argv[0]) - OOP_INT(argv[1])));
}
PRIM(__Integer_mutiply_)
{
  ft_vm_push(vm, INT_OOP(OOP_INT(argv[0]) * OOP_INT(argv[1])));
}
PRIM(__Integer_divide_)
{
  ft_vm_push(vm, INT_OOP(OOP_INT(argv[0]) / OOP_INT(argv[1])));
}
PRIM(__Integer_modulo_)
{
  ft_vm_push(vm, INT_OOP(OOP_INT(argv[0]) % OOP_INT(argv[1])));
}
PRIM(__Integer_print)
{
  printf("%ld", OOP_INT(argv[0]));
  ft_vm_push(vm, argv[0]);
}
PRIM(__Integer_printLn)
{
  printf("%ld\n", OOP_INT(argv[0]));
  ft_vm_push(vm, argv[0]);
}

PRIM(__VTable_instantiate)
{
  // TODO
  struct ft_vtable* vt = (struct ft_vtable*)argv[0];
  oop obj = ft_vtable_instantiate(ft_environment_allocator(env), vt);
  ft_vtable_debug_print(vt);
  ft_vm_push(vm, obj);
}

PRIM(__VM_activateMethodContext)
{
  oop receiver = argv[0];
  if(!receiver)
  {
    fprintf(stderr, "__VM_activateMethodContext :: receiver cannot be null\n");
    ft_vm_push(vm, OOP_NIL);
    return;
  }
  oop code = ft_vtable_get_code(ft_object_vtable(argv[0]));
  if(code)
  {
    ft_vm_push_frame(vm, argv[0]);
  }
  else
  {
    ft_vm_push(vm, OOP_NIL);
  }
}

#undef PRIM


#define PRIMNAME(name) \
  ft_primitive_##name

void ft_primitives_init(struct ft_environment* env)
{
#define INTERN(s) ft_environment_intern(env, (s))
  struct ft_environment_primitive_method prims[] = {
    {INTERN("__Lobby"), PRIMNAME(__Lobby)},
    {INTERN("__Object_printSlots"), PRIMNAME(__Object_printSlots)},
    {INTERN("__Object_respondsTo:"), PRIMNAME(__Object_respondsTo_)},
    {INTERN("__Object_copyAddingSlot:value:"), PRIMNAME(__Object_copyAddingSlot_value_)},
    {INTERN("__Object_addStaticSlot:value:"), PRIMNAME(__Object_addStaticSlot_value_)},
    {INTERN("__Object_setSlot:"), PRIMNAME(__Object_setSlot_)},
    {INTERN("__String_concat:"), PRIMNAME(__String_concat_)},
    {INTERN("__String_readFile"), PRIMNAME(__String_readFile)},
    {INTERN("__String_compileAnonymousMethod"), PRIMNAME(__String_compileAnonymousMethod)},
    {INTERN("__String_printLn"), PRIMNAME(__String_printLn)},
    {INTERN("__Integer_add:"), PRIMNAME(__Integer_add_)},
    {INTERN("__Integer_subtract:"), PRIMNAME(__Integer_subtract_)},
    {INTERN("__Integer_multiply:"), PRIMNAME(__Integer_mutiply_)},
    {INTERN("__Integer_divide:"), PRIMNAME(__Integer_divide_)},
    {INTERN("__Integer_modulo:"), PRIMNAME(__Integer_modulo_)},
    {INTERN("__Integer_print"), PRIMNAME(__Integer_printLn)},
    {INTERN("__String_print"), PRIMNAME(__String_print)},
    {INTERN("__Integer_printLn"), PRIMNAME(__Integer_printLn)},
    {INTERN("__VTable_instantiate"), PRIMNAME(__VTable_instantiate)},
    {INTERN("__VM_activateMethodContext"), PRIMNAME(__VM_activateMethodContext)}
  };

  ft_environment_set_primitives(env, sizeof(prims) / sizeof(struct ft_environment_primitive_method), prims);
#undef INTERN
}
#undef PRIMNAME