#include "fowltalk.h"
#include "fowltalk/environment.h"
#include "fowltalk/array.h"
#include "fowltalk/bytes.h"
#include "fowltalk/vtable.h"
#include "fowltalk/string.h"
#include "fowltalk/compiler.h"
#include "object-header.h"
#include "memory.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

struct ft_environment
{
  struct ft_allocator allocator;
  ft_object string_map;
  ft_object lobby;

  int primitives_capacity_mask;
  struct ft_environment_primitive_method* primitives;
};

enum ft_environment_string_map_slots
{
  ftesm_size,
  ftesm_contents,
  ftesm_string_vtable
};

ft_primitive_t ft_environment_primitive(struct ft_environment* env, struct ft_string* selector)
{
  int hash = ft_string_hash(selector);
  for(int i = hash & env->primitives_capacity_mask; env->primitives[i].selector; i = (i + 1) & env->primitives_capacity_mask)
  {
    struct ft_string* this_selector = env->primitives[i].selector;
    if(ft_string_hash(this_selector) == hash && ft_string_compare(selector, this_selector) == 0)
      return env->primitives[i].function;
  }
  return NULL;
}

void ft_environment_set_primitives(struct ft_environment* env, int primitive_count, struct ft_environment_primitive_method* methods)
{
  for(int i = 0; i < primitive_count; ++i)
  {
    struct ft_environment_primitive_method* prim = methods + i;
    int hash = ft_string_hash(prim->selector);
    int capacity_mask = env->primitives_capacity_mask;
    int index = hash & capacity_mask;
    for(; env->primitives[index].selector; index = (index + 1) & capacity_mask)
    {
      struct ft_environment_primitive_method* method = env->primitives + index;
      if(ft_string_hash(method->selector) == hash && ft_string_compare(method->selector, prim->selector) == 0)
      {
        method->function = prim->function;
        goto next_prim;
      }
    }
    env->primitives[index] = *prim;
  next_prim:
    (void)0;
  }
}


void ft_environment_set_interned_string_vtable(struct ft_environment* env, struct ft_vtable* vtable)
{
  ft_integer_t size = OOP_INT(env->string_map[ftesm_size]),
    updated = 0;
  for(oop* iter = ft_array_items_begin((struct ft_array*)env->string_map[ftesm_contents]), *end = iter + ft_array_size((struct ft_array*)env->string_map[ftesm_contents]);
    updated < size && iter < end;
    ++iter )
  {
    if(*iter)
    {
      ft_set_vtable(*iter, vtable);
      ++updated;
    }
  }
  env->string_map[2] = vtable;
}


void print_error_exit(struct ft_virtual_machine* vm)
{
  char error_string_buffer[256];
  ft_vm_describe_error(vm, error_string_buffer, 256);
  fprintf(stderr, "execution error: %s", error_string_buffer);
  exit(1);
}

void ft_init_environment(struct ft_environment* env, int mmap_size, int primitive_capacity)
{
  ft_init_allocator(&env->allocator, mmap_size);
  env->primitives_capacity_mask = primitive_capacity - 1;
  env->primitives = (struct ft_environment_primitive_method*)calloc(sizeof(struct ft_environment_primitive_method), primitive_capacity);
}

int nil_vtable(oop value)
{
  return !ft_object_vtable(value);
}

void print_object_count(oop value, void* arg)
{
  printf("  %p\n", value);
  int *count = (int*)arg;
  ++*count;
}

void print_nil_vtables(struct ft_environment* env)
{
  int count = 0;
  ft_allocator_each_object(ft_environment_allocator(env), nil_vtable, print_object_count, &count);
  printf("%d total nil vtable objects\n", count);
}

/* Generate the minimum object structure for the compiler to work

Globals

- StringMap // a hash map of interned strings
  - size
  - contents

- VTableVT // [DEPRECATED] the vtable for all vtables
- VTable // the vtable for all vtables


- VirtualMachineVT // vtable used for vm instances


- BytecodePrototype // 
  - instructions // a Bytes with compiled instructions
  - literals // an Array for literals 

- Bytes - size 0 Byte Array prototype
- Array - size 0 Array prototype

*/
struct ft_environment* ft_new_minimal_environment()
{
  struct ft_environment* env = (struct ft_environment*)calloc(sizeof(struct ft_environment), 1);
  ft_init_environment(env, 1 << 20, 256);

  struct ft_allocator* allocator = ft_environment_allocator(env);

#define STR(name) \
  struct ft_string* s_##name = ft_new_string(allocator, NULL, strlen(#name), #name)

  // STR(StringVT);
  STR(size);
  STR(contents);
  STR(memberVTable);

  // print_nil_vtables(env);

  struct ft_vtable* string_map_vt = NULL;

  {
    // StringMap, this object is a hash map for interned strings
    struct ft_vtable_slot_desc string_map_slots[] = {
      {s_size, ftvs_data_slot, NULL},
      {s_contents, ftvs_data_slot, NULL},
      {s_memberVTable, ftvs_data_slot, NULL}
    };
    string_map_vt = ft_new_vtable(allocator, NULL, 2, string_map_slots, 0);
    env->string_map = (ft_object)ft_vtable_instantiate(allocator, string_map_vt);

    const int capacity = 128;
    const int capacity_mask = capacity - 1;

    struct ft_string* initial_strings[3] = {
      s_size, s_contents, s_memberVTable
    };

    env->string_map[ftesm_size] = INT_OOP(3);
    env->string_map[ftesm_contents] = ft_new_array(allocator, NULL, capacity);

    ft_object buckets = (ft_object)ft_array_items_begin(env->string_map[ftesm_contents]);

    for(int i = 0;
        i < (int)(sizeof(initial_strings) / sizeof(initial_strings[0]));
        ++i)
    {
      struct ft_string* key = initial_strings[i];
      int hash = ft_string_hash(key);
      int index = hash & capacity_mask;
      for(; buckets[index]; index = (index + 1) & capacity_mask)
        ;
      buckets[index] = key;
    }

  }

  // printf("--01---\n");
  // print_nil_vtables(env);

#undef STR
#define STR(s) \
  struct ft_string* s_##s = ft_environment_intern(env, #s) // ft_new_string(allocator, string_vt, strlen(#s), #s)

  STR(instanceSize);
  STR(VTable);
  STR(typeName);
  STR(String);

  struct ft_vtable_slot_desc vtable_slots[2] = {
    {s_instanceSize, ftvs_static_slot, OOP_NIL},
    {s_typeName, ftvs_static_slot, s_VTable}
  };

  struct ft_vtable* vtable_vt = ft_new_vtable(allocator, NULL, 1, vtable_slots, 0);
  ft_set_vtable(vtable_vt, vtable_vt);
  ft_set_vtable(string_map_vt, vtable_vt);
  ft_vtable_debug_print(vtable_vt);

  // create a StringVT and set all interned strings to use it
  struct ft_vtable_slot_desc string_slots[] = {
    {s_typeName, ftvs_static_slot, s_String},
    {NULL, 0, NULL}
  };

  struct ft_vtable* string_vt = ft_new_vtable(allocator, vtable_vt, -1, string_slots, 0);
  // ft_vtable_debug_print(string_vt);

  ft_environment_set_interned_string_vtable(env, string_vt);

  // struct ft_vtable* lobby_vt = ft_new_vtable(allocator, NULL, -1, lobby_slots, 0);
  // oop lobby = ft_vtable_instantiate(allocator, lobby_vt);

  ft_primitives_init(env);


  STR(Array);
  STR(Bytes);
  STR(ByteCode);
  STR(instructions);
  STR(literals);

  // ft_allocator_debug_print(allocator);
  // printf("---\n");

  struct ft_vtable* vm_vt = ft_new_vtable(allocator, vtable_vt, 0, NULL, ft_vm_struct_size());

  struct ft_vtable_slot_desc array_slots[] = {
    {s_size, ftvs_static_slot, INT_OOP(0)},
    {s_typeName, ftvs_static_slot, s_Array}
  };
  struct ft_vtable* array_vt = ft_new_vtable(allocator, vtable_vt, 2, array_slots, 0);
  ft_object_header(env->string_map[ftesm_contents])->base.vtable = array_vt;

  struct ft_vtable_slot_desc byte_slots[] = {
    {s_typeName, ftvs_static_slot, s_Bytes}
  };
  struct ft_vtable* bytes_vt = ft_new_vtable(allocator, vtable_vt, 1, byte_slots, 0);

  struct ft_vtable_slot_desc bytecode_slots[] = {
    {s_instructions, ftvs_data_slot, NULL},
    {s_literals, ftvs_data_slot, NULL},
    {s_typeName, ftvs_static_slot, s_ByteCode},
  };
  struct ft_vtable* bytecode_vt = ft_new_vtable(allocator, vtable_vt, 2, bytecode_slots, 0);

  ft_object bytecode_proto = (ft_object)ft_vtable_instantiate(allocator, bytecode_vt);
  bytecode_proto[0] = ft_new_bytes(allocator, bytes_vt, 8);
  bytecode_proto[1] = ft_new_array(allocator, array_vt, 2);

  STR(BytecodePrototype);
  STR(VirtualMachineVT);
  STR(VTableVT);

  struct ft_vtable_slot_desc lobby_slots[20] = {
    {NULL, 0, NULL}
  };

  struct ft_vtable_slot_desc *lobby_slot_p = lobby_slots;
  ft_vtable_slot_desc_init(lobby_slot_p++, ftvs_static_slot, s_String, ft_new_string(ft_environment_allocator(env), string_vt, 0, ""));
  ft_vtable_slot_desc_init(lobby_slot_p++, ftvs_static_slot, s_BytecodePrototype, bytecode_proto);
  ft_vtable_slot_desc_init(lobby_slot_p++, ftvs_static_slot, s_VirtualMachineVT, vm_vt);
  ft_vtable_slot_desc_init(lobby_slot_p++, ftvs_static_slot, s_VTableVT, vtable_vt);
  ft_vtable_slot_desc_init(lobby_slot_p++, ftvs_static_slot, s_VTable, vtable_vt);
  ft_vtable_slot_desc_init(lobby_slot_p++, ftvs_static_slot, s_Array,
    ft_new_array(ft_environment_allocator(env), array_vt, 0));
  ft_vtable_slot_desc_init(lobby_slot_p++, ftvs_static_slot, s_Bytes,
    ft_new_bytes(ft_environment_allocator(env), bytes_vt, 0));

  ft_vtable_slot_desc_init(lobby_slot_p++, 0, NULL, NULL);

  struct ft_vtable* lobby_vt = ft_new_vtable(allocator, vtable_vt, -1, lobby_slots, 0);
  oop lobby = ft_vtable_instantiate(allocator, lobby_vt);

  env->lobby = lobby;

#undef STR

  // print_nil_vtables(env);

  // ft_integer_t capacity = ft_array_size(env->string_map[ftesm_contents]); // OOP_INT(env->string_map[ftesm_size]);
  // ft_object items = ft_array_items_begin(env->string_map[ftesm_contents]);
  // for(ft_integer_t i = 0; i < capacity; ++i)
  // {
  //   if(items[i])
  //   {
  //     ft_set_vtable(items[i], string_vt);
  //   }
  // }

  // printf("------\n");
  // print_nil_vtables(env);

  return env;
}


oop ft_compile_file_path(struct ft_environment* env, struct ft_compiler_options* opt, const char* path)
{
  FILE* file = fopen(path, "r");
  oop method = ft_compile_file(env, opt, file);
  fclose(file);

  return method;
}


struct ft_environment* ft_new_environment(const char* dir, const char* platform_name)
{
  struct ft_environment* env = ft_new_minimal_environment();

  struct ft_allocator* allocator = ft_environment_allocator(env);

  oop bytecode_proto = ft_simple_lookup(env->lobby, ft_environment_intern(env, "BytecodePrototype"));
  struct ft_vtable* vtable_vt = (struct ft_vtable*)ft_simple_lookup(env->lobby, ft_environment_intern(env, "VTable"));

  struct ft_vtable* vm_vt = (struct ft_vtable*)ft_simple_lookup(env->lobby, ft_environment_intern(env, "VirtualMachineVT"));

  struct ft_compiler_options compiler_options;
  // compiler_options.bytecode_prototype = bytecode_proto;
  // compiler_options.vtable_vt = vtable_vt;
  ft_init_compiler_options(env, &compiler_options);


#define STR(s) \
  struct ft_string* s_##s = ft_environment_intern(env, #s) // ft_new_string(allocator, string_vt, strlen(#s), #s)

  char platform_path[1024];
  int platform_path_written = snprintf(platform_path, 1024, "%s/%s.ftl", dir, platform_name);
  // TODO check that ^

  {
    oop method = ft_compile_file_path(env, &compiler_options, platform_path);

    if(!method)
    {
      fprintf(stderr, "failed to load environment '%s' from file '%s'\n", platform_name, platform_path);
    }
    else
    {
      struct ft_vm_quick_send_options opt = {env, vm_vt, 1024};
      oop result = ft_vm_quick_activate_and_run(&opt, method, OOP_NIL, print_error_exit);
      if(result)
      {
        env->lobby = result;
      }
      else
      {
        fprintf(stderr, "failed to load lobby platform from file '%s'\n", platform_path);
      }
    }
  }

  return env;

#undef STR
}




struct ft_allocator* ft_environment_allocator(struct ft_environment* env)
{
  return &env->allocator;
}

oop ft_environment_lobby(struct ft_environment* env)
{
  return env->lobby;
}

void ft_environment_grow_string_map(struct ft_environment* env)
{
  struct ft_array* old_buckets = (struct ft_array*)env->string_map[ftesm_contents];
  int old_capacity = ft_array_size(old_buckets);

  int new_capacity = old_capacity * 2,
    new_capacity_mask = new_capacity - 1;
  struct ft_array* new_buckets = ft_new_array(ft_environment_allocator(env), ft_object_vtable(old_buckets), new_capacity);
  oop* new_buckets_items = ft_array_items_begin(new_buckets);

  ft_integer_t remaining = OOP_INT(env->string_map[ftesm_size]);
  for( oop *iter = ft_array_items_begin(old_buckets), *end = iter + old_capacity;
    remaining && iter < end;
    ++iter )
  {
    if(*iter)
    {
      int hash = ft_string_hash((struct ft_string*)*iter);
      int index = hash & new_capacity_mask;
      for(; new_buckets_items[index];
        index = (index + 1) & new_capacity_mask)
      {
      }
      new_buckets_items[index] = *iter;
      --remaining;
    }
  }

  env->string_map[ftesm_contents] = new_buckets;
}

struct ft_string* ft_environment_intern(struct ft_environment* env, const char* str)
{
  int hash = djb2(str);
  struct ft_array* contents = (struct ft_array*)env->string_map[ftesm_contents];
  ft_object buckets = ft_array_items_begin(contents);
  int capacity_mask = ft_array_size(contents) - 1;
  int index = hash & capacity_mask;
  for(; buckets[index]; index = (index + 1) & capacity_mask)
  {
    struct ft_string* this_str = (struct ft_string*)buckets[index];
    if(ft_string_hash(this_str) == hash && strcmp(ft_string_bytes(this_str), str) == 0)
    {
      return this_str;
    }
  }

  struct ft_vtable* string_vt = (struct ft_vtable*)env->string_map[ftesm_string_vtable];
  struct ft_string* new_string = ft_new_string(ft_environment_allocator(env), string_vt, strlen(str), str);
  buckets[index] = new_string;
  ft_integer_t size = OOP_INT(env->string_map[ftesm_size]);
  env->string_map[ftesm_size] = INT_OOP(size+1);

  return new_string;
}

