#ifndef FT_DEBUG_FOWLTALK
#define FT_DEBUG_FOWLTALK 0
#endif

#define FT_DEBUG FT_DEBUG_FOWLTALK
#include "fowltalk/debug.h"

#include "fowltalk.h"
#include "object-header.h"
#include "memory.h"
#include "fowltalk/environment.h"
#include "fowltalk/bytes.h"
#include "fowltalk/vtable.h"
#include "fowltalk/string.h"
#include "fowltalk/compiler.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>





oop ft_perform0(oop receiver, struct ft_string* selector)
{
  oop method = OOP_NIL;
  ft_integer_t setter_index = -1;
  enum ft_lookup_result lookup_result = ft_lookup(receiver, selector, NULL, &method, &setter_index);
  switch(lookup_result)
  {
  case ftlr_not_found:
    return OOP_NIL;

  case ftlr_data:
    return method;

  case ftlr_setter:
    // what to do here?? no value to set it to, this shouldn't happen
    fprintf(stderr, "ft_perform0 called on a setter with no arg");
    exit(204);
    return OOP_NIL;
  }

}

oop ft_simple_lookup(oop receiver, struct ft_string* selector)
{
  oop method = OOP_NIL;
  ft_integer_t setter_index = -1;
  enum ft_lookup_result lookup_result = ft_lookup(receiver, selector, NULL, &method, &setter_index);
  switch(lookup_result)
  {
  case ftlr_data:
    return method;

  case ftlr_setter:
  case ftlr_not_found:
    return OOP_NIL;
  }
}

enum ft_lookup_result ft_lookup(oop object, struct ft_string* selector, oop* context, oop* result, ft_integer_t* setter_index)
{
  oop search_stack[32];
  memset(search_stack, 0, sizeof(search_stack));
  oop* search_stack_end = search_stack + 32;
  oop* search_next = search_stack;
  *search_next++ = object;

  ft_debug("beginning lookup '%s'", ft_string_bytes(selector));

  for(oop* sp = search_stack; sp < search_stack_end && *sp; ++sp)
  {
    oop obj = *sp;
    for(oop* iter = search_stack; iter < sp; ++iter)
    {
      if(*iter == obj)
      {
        goto next_obj;
      }
    }


    struct ft_vtable* vtable = ft_object_vtable(obj);

    ft_debug("  looking in %p", obj);
    ft_vtable_debug_print(vtable);

    struct ft_vtable_slot* vt_slot = ft_vtable_lookup(vtable, selector);
    if(vt_slot)
    {
      if(context)
        *context = obj;

      oop value = ft_vtable_slot_value(vt_slot);
      switch(ft_vtable_slot_type(vt_slot))
      {
      case ftvs_parent_slot:
      case ftvs_data_slot:
        *result = ((ft_object)obj)[OOP_INT(value)];
        return ftlr_data;

      case ftvs_static_slot:
        *result = value;
        return ftlr_data;

      case ftvs_static_parent:
        *result = ft_vtable_static_parents_begin(vtable)[OOP_INT(value)];
        return ftlr_data;

      case ftvs_setter_slot:
        *setter_index = OOP_INT(value);
        return ftlr_setter;
      
      }
    }

    // add parents
    for(oop* p = ft_vtable_parents_begin(vtable, obj);
        p < ft_vtable_parents_end(vtable, obj);
        ++p)
      *search_next++ = *p;

    for(oop* p = ft_vtable_static_parents_begin(vtable);
        p < ft_vtable_static_parents_end(vtable);
        ++p)
      *search_next++ = *p;

  next_obj:
    (void)0;
  }

  *result = OOP_NIL;
  return ftlr_not_found;
}







// oop ft_init(struct ft_environment* env, const char* dir, const char* platform_name)
// {
//   struct ft_allocator* allocator = ft_environment_allocator(env);


//   struct ft_vtable* string_vt = NULL;

// #define STR(s) struct ft_string* s_##s = ft_environment_intern(env, #s) // ft_new_string(allocator, string_vt, strlen(#s), #s)

//   STR(instanceSize);
//   STR(isNil);
//   STR(super);
//   STR(size);
//   STR(array);
//   STR(bytes);
//   STR(typeName);
//   STR(instructions);
//   STR(literals);

//   // ft_allocator_debug_print(allocator);
//   // printf("---\n");

//   struct ft_vtable_slot_desc vtable_slots[1] = {
//     {s_instanceSize, ftvs_static_slot, OOP_NIL}
//   };

//   struct ft_vtable* vtable_vt = ft_new_vtable(allocator, NULL, 1, vtable_slots, 0);
//   ft_vtable_debug_print(vtable_vt);




//   struct ft_vtable_slot_desc array_slots[] = {
//     {s_size, ftvs_static_slot, INT_OOP(0)},
//     {s_typeName, ftvs_static_slot, s_array}
//   };
//   struct ft_vtable* array_vt = ft_new_vtable(allocator, vtable_vt, 2, array_slots, 0);


//   struct ft_vtable_slot_desc byte_slots[] = {
//     {s_typeName, ftvs_static_slot, s_bytes}
//   };
//   struct ft_vtable* bytes_vt = ft_new_vtable(allocator, vtable_vt, 1, byte_slots, 0);

//   struct ft_vtable_slot_desc bytecode_slots[] = {
//     {s_instructions, ftvs_data_slot},
//     {s_literals, ftvs_data_slot}
//   };
//   struct ft_vtable* bytecode_vt = ft_new_vtable(allocator, vtable_vt, 2, bytecode_slots, 0);
//   ft_object bytecode = (ft_object)ft_vtable_instantiate(allocator, bytecode_vt);

//   bytecode[0] = ft_new_bytes(allocator, bytes_vt, 8 * 4);
//   bytecode[1] = ft_new_array(allocator, array_vt, 4);

//   char platform_path[1024];
//   int platform_path_written = snprintf(platform_path, 1024, "%s/%s.ftl", dir, platform_path);
//   // TODO check that ^

//   FILE* platform_file = fopen(platform_path, "r");
//   oop method = ft_compile_file(env, platform_file, bytecode);
//   fclose(platform_file);




//   struct ft_vtable_slot_desc root_slots[1] = {
//     {s_isNil, ftvs_static_slot, INT_OOP(99)}
//   };

//   // ft_allocator_debug_print(allocator);
//   // printf("---\n");

//   struct ft_vtable* root_vt = ft_new_vtable(allocator, vtable_vt, 1, root_slots, 0);
//   ft_vtable_debug_print(root_vt);

//   oop root = ft_vtable_instantiate(allocator, root_vt);

//   struct ft_vtable_slot_desc string_slots[2] = {
//     {s_super, ftvs_static_parent, root}
//   };

//   string_vt = ft_new_vtable(allocator, vtable_vt, 1, string_slots, 0);
//   ft_vtable_debug_print(string_vt);

//   struct ft_string* fix_strings[] = {s_instanceSize, s_isNil, s_super, s_size, NULL};
//   for(struct ft_string** str = fix_strings; *str; ++str)
//     ft_object_header(*str)->vtable = string_vt;

//   STR(x);
//   STR(y);

//   struct ft_vtable_slot_desc point_slots[3] = {
//     {s_x, ftvs_data_slot},
//     {s_y, ftvs_data_slot},
//     {s_super, ftvs_static_parent, root}
//   };

//   struct ft_vtable* point_vt = ft_new_vtable(allocator, vtable_vt, 3, point_slots, 0);
//   ft_vtable_debug_print(point_vt);

//   oop point1 = ft_vtable_instantiate(allocator, point_vt);
//   ((ft_object)point1)[0] = INT_OOP(42);
//   oop point1_x = ft_lookup(point1, s_x, NULL);
//   assert(OOP_IS_INT(point1_x));
//   assert(OOP_INT(point1_x) == 42);

//   oop point1_isNil = ft_lookup(point1, s_isNil, NULL);
//   assert(OOP_IS_INT(point1_isNil));
//   assert(OOP_INT(point1_isNil) == 99);

//   return OOP_NIL;

// #undef STR
// }






oop ft_clone(struct ft_allocator* allocator, oop value)
{
  if(!OOP_IS_PTR(value))
    return value;

  int size = ft_object_actual_size_bytes(value);
  oop new_object = ft_allocator_alloc(allocator, ft_object_vtable(value), size);
  memcpy(new_object, value, size);
  return new_object;
}

oop ft_deep_clone(struct ft_allocator* allocator, oop value, int depth)
{
  if(!depth-- || !OOP_IS_PTR(value))
    return value;

  oop new_object = ft_clone(allocator, value);
  struct ft_vtable* vt = ft_object_vtable(new_object);
  for(oop* slot = ft_vtable_data_slots_begin(vt, new_object), *end = ft_vtable_data_slots_end(vt, new_object);
    slot < end;
    ++slot)
  {
    *slot = ft_deep_clone(allocator, *slot, depth);
  }
  return new_object;
}




