#ifndef FT_DEBUG_VTABLE
#define FT_DEBUG_VTABLE 0
#endif

#define FT_DEBUG FT_DEBUG_VTABLE
#include "fowltalk/debug.h"

#include "fowltalk.h"
#include "fowltalk/vtable.h"
#include "fowltalk/string.h"
#include "object-header.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

struct ft_vtable_slot
{
  struct ft_string* name;
  oop value;
};

void ft_vtable_slot_init(struct ft_vtable_slot* slot, enum ft_vtable_slot_type slot_type, struct ft_string* name, oop value)
{
  slot->name = (struct ft_string*)((intptr_t)name | (slot_type & ftvs__mask));
  slot->value = value;
}

enum ft_vtable_slot_type ft_vtable_slot_type(struct ft_vtable_slot* slot)
{
  return (intptr_t)(slot->name) & ftvs__mask;
}

struct ft_string* ft_vtable_slot_name(struct ft_vtable_slot* slot)
{
  return (struct ft_string*)((intptr_t)(slot->name) & ~(intptr_t)ftvs__mask);
}

oop ft_vtable_slot_value(struct ft_vtable_slot* slot)
{
  return slot->value;
}

struct ft_vtable_slot* ft_vtable_slot_next(struct ft_vtable_slot* slot)
{
  return slot+1;
}






void ft_vtable_slot_desc_init(struct ft_vtable_slot_desc* slot, enum ft_vtable_slot_type slot_type, struct ft_string* name, oop value)
{
  slot->name = name;
  slot->type = slot_type;
  slot->value = value;
}

struct ft_vtable
{
  int instance_size, slots_begin, slots_count, parents_count,
    capacity_mask, static_parents_begin, static_parents_count,
    flags;
  oop code;
  struct ft_vtable_slot slots[0];
};

struct ft_vtable_slot* ft_vtable_slots_begin(struct ft_vtable* vt)
{
  return vt->slots;
}

struct ft_vtable_slot* ft_vtable_slots_end(struct ft_vtable* vt)
{
  return vt->slots + vt->capacity_mask + 1;
}

oop* ft_vtable_parents_begin(struct ft_vtable* vt, oop value)
{
  return (ft_object)value + vt->slots_begin;
}

oop* ft_vtable_parents_end(struct ft_vtable* vt, oop value)
{
  return ft_vtable_parents_begin(vt, value) + vt->parents_count;
}

oop* ft_vtable_data_slots_begin(struct ft_vtable* vt, oop value)
{
  return (ft_object)value + vt->slots_begin;
}

oop* ft_vtable_data_slots_end(struct ft_vtable* vt, oop value)
{
  return ft_vtable_data_slots_begin(vt, value) + vt->slots_count;
}

oop* ft_vtable_static_parents_begin(struct ft_vtable* vt)
{
  return (oop*)ft_vtable_slots_end(vt);
}

oop* ft_vtable_static_parents_end(struct ft_vtable* vt)
{
  return ft_vtable_static_parents_begin(vt) + vt->static_parents_count;
}


int ft_vtable_slot_capacity(struct ft_vtable* vt)
{
  return vt->capacity_mask + 1;
}

int ft_vtable_base_size(struct ft_vtable* vt)
{
  return vt->slots_begin;
}


// fill a buffer of ft_vtable_slot with the slots from this vtable
// order is not preserved, this list of slots is not suitable for use with ft_new_vtable(),
// see ft_vtable_decompose_slots() for that
int ft_vtable_list_raw_slots(struct ft_vtable* vt, int buffer_size, struct ft_vtable_slot* slots, int slot_offset)
{
  int index = 0, result = 0;
  int vt_capacity = ft_vtable_slot_capacity(vt);
  for(; slot_offset && index < vt_capacity; ++index)
  {
    struct ft_vtable_slot* slot = ft_vtable_slots_begin(vt) + index;
    struct ft_string* name = ft_vtable_slot_name(slot);
    if(name)
    {
      --slot_offset;
    }
  }
  for(; buffer_size && index < vt_capacity; ++index)
  {
    struct ft_vtable_slot* slot = ft_vtable_slots_begin(vt) + index;
    struct ft_string* name = ft_vtable_slot_name(slot);
    if(name)
    {
      *slots++ = *slot;
      ++result;
      --buffer_size;
    }
  }
  return result;
}

// decompose vtable to a list of vtable_slot_desc
// buffer_size must be at least as big as the capacity of the vtable
// the resulting slot list will be in the right order to make an object with identical layout to `vt`
// return value is the total number of slots read
int ft_vtable_decompose_slots(struct ft_vtable* vt, int buffer_size, struct ft_vtable_slot_desc* slots)
{
  int vt_capacity = ft_vtable_slot_capacity(vt);

  struct ft_vtable_slot* slot_buffer = calloc(sizeof(struct ft_vtable_slot), vt_capacity);
  int slot_count = ft_vtable_list_raw_slots(vt, vt_capacity, slot_buffer, 0);
  int static_slots_written = 0, setter_slots = 0;

  for(int i = 0; i < slot_count; ++i)
  {
    struct ft_vtable_slot* slot = slot_buffer+i;
    enum ft_vtable_slot_type type = ft_vtable_slot_type(slot);
    struct ft_string* name = ft_vtable_slot_name(slot);
    oop value = ft_vtable_slot_value(slot);
    switch(type)
    {
    case ftvs_data_slot:
    case ftvs_parent_slot:
      ft_vtable_slot_desc_init(
        slots + OOP_INT(value) - vt->slots_begin,
        type,
        name,
        OOP_NIL);
      break;

    case ftvs_static_parent:
      ft_vtable_slot_desc_init(
        slots + vt->slots_count + OOP_INT(value) - vt->slots_begin,
        type,
        name,
        ft_vtable_static_parents_begin(vt)[ OOP_INT(value) ]);
      break;

    case ftvs_static_slot:
      ft_vtable_slot_desc_init(
        slots + vt->slots_count + vt->static_parents_count + static_slots_written - vt->slots_begin,
        type,
        name,
        value);
      ++static_slots_written;
      break;

    case ftvs_setter_slot:
      ft_vtable_slot_desc_init(
        slots + vt->slots_count + vt->static_parents_count + static_slots_written - vt->slots_begin,
        type,
        name,
        value); // value here needs to be converted to the string field name
      ++static_slots_written;
      ++setter_slots;
      break;
    }
  }

  for(int i = vt->slots_count; setter_slots && i < slot_count; ++i)
  {
    struct ft_vtable_slot_desc* slot = slots + i;
    if(slot->type == ftvs_setter_slot)
    {
      struct ft_vtable_slot_desc* target = slots + OOP_INT(slot->value);
      slot->value = target->name;
      --setter_slots;
    }
  }

  free(slot_buffer);
  return slot_count;
}

struct ft_vtable_slot* ft_vtable_lookup(struct ft_vtable* vt, struct ft_string* selector)
{
  int hash = ft_string_hash(selector);
  int index = hash & vt->capacity_mask;
  while(1)
  {
    struct ft_vtable_slot* slot = ft_vtable_slots_begin(vt) + index;
    struct ft_string* name = ft_vtable_slot_name(slot);
    if(!name)
      break;
    if(ft_string_hash(name) == hash && ft_string_compare(name, selector) == 0)
    {
      return slot;
    }

    index = (index + 1) & vt->capacity_mask;
  }
  return NULL;
}

void ft_vtable_each_slot(struct ft_vtable* vt, void(*callback)(enum ft_vtable_slot_type, struct ft_string*, oop))
{
  int capacity = vt->capacity_mask + 1;
  for(int i = 0; i < capacity; ++i)
  {
    struct ft_vtable_slot* slot = vt->slots + i;
    struct ft_string* name = ft_vtable_slot_name(slot);
    if(name)
    {
      callback(ft_vtable_slot_type(slot), name, ft_vtable_slot_value(slot));
    }
  }
}

void ft_vtable_debug_print(struct ft_vtable* vt)
{
  int capacity = vt->capacity_mask + 1;

  const char* sep = "";
  char slots_str[1024];
  char* slots_str_p = slots_str;
  for(int i = 0; i < capacity; ++i)
  {
    struct ft_string* name = ft_vtable_slot_name(vt->slots+i);
    if(name)
    {
      int result = snprintf(slots_str_p, 1024 - (slots_str_p - slots_str), "%s%s", sep, ft_string_bytes(name));
      slots_str_p += result;
      sep = ", ";
    }
  }

  ft_debug("{vt instanceSize=%d slotsBegin=%d slotsCount=%d parentsCount=%d "
    "slotCapacity=%d staticParentsBegin=%d staticParentsCount=%d slots=[%s]}",
    vt->instance_size, vt->slots_begin, vt->slots_count, vt->parents_count,
    capacity, vt->static_parents_begin, vt->static_parents_count, slots_str);
}

void ft_vtable_print_stats(struct ft_vtable* vt)
{
  printf("instanceSize=%d slotsCount=%d parentsCount=%d slotCapacity=%d staticParentsCount=%d",
    vt->instance_size, vt->slots_count, vt->parents_count, vt->capacity_mask+1, vt->static_parents_count);
}


uint32_t next_power_of_two(uint32_t v)
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

int vtable_insert(struct ft_vtable* vt, enum ft_vtable_slot_type type, struct ft_string* key, oop value)
{
  int hash = ft_string_hash(key);
  int index = hash & vt->capacity_mask;
  while(1)
  {
    struct ft_vtable_slot* slot = vt->slots + index;
    struct ft_string* name = ft_vtable_slot_name(slot);
    if(!name)
    {
      // insert here
      ft_vtable_slot_init(slot, type, key, value);
      return 1;
    }

    index = (index + 1) & vt->capacity_mask;
  }
  return 0;
}

int vtable_static_update(struct ft_vtable* vt, struct ft_string* key, oop value)
{
  int hash = ft_string_hash(key);
  int index = hash & vt->capacity_mask;
  while(1)
  {
    struct ft_vtable_slot* slot = vt->slots + index;
    struct ft_string* name = ft_vtable_slot_name(slot);
    if(ft_string_hash(name) == hash && ft_string_compare(key, name) == 0)
    {
      enum ft_vtable_slot_type type = ft_vtable_slot_type(slot);
      if(type & ftvs_static_slot)
      {
        if(type & ftvs_parent_slot)
        {
          ft_integer_t index = OOP_INT(ft_vtable_slot_value(slot));
          ft_vtable_static_parents_begin(vt)[index] = value;
        }
        else
        {
          ft_vtable_slot_init(slot, type, name, value);
        }
        return 1;
      }

      return 0;
    }

    index = (index + 1) & vt->capacity_mask;
  }
  return 0;
}

struct ft_vtable* ft_new_method_vtable(struct ft_allocator* allocator, struct ft_vtable* vtable, int slot_count, struct ft_vtable_slot_desc* slots, oop code)
{
  if(!vtable)
  {
    fprintf(stderr, "vtable is null\n");
  }
  struct ft_vtable* vt = ft_new_vtable(allocator, vtable, slot_count, slots, 0);
  vt->flags |= ftvf_activatable_method;
  vt->code = code;
  return vt;
}


bool ft_string_equals(struct ft_string* a, struct ft_string* b)
{
  return (a == b) || (ft_string_compare(a, b) == 0);
}


struct vt_tmp_data_slot
{
  struct ft_string* slot_name;
  ft_integer_t index;
};

struct vt_tmp_setter_slot
{
  struct ft_string *slot_name, *slot_to_set;
};

struct ft_vtable* ft_new_vtable(struct ft_allocator* allocator, struct ft_vtable* vtable, int slot_count, struct ft_vtable_slot_desc* slots, int base_size)
{
  if(slot_count == -1)
  {
    // count slots until the first NULL name
    slot_count = 0;
    for(struct ft_vtable_slot_desc* slot = slots;
        slot->name;
        ++slot )
    {
      ++slot_count;
    }
  }
  int vt_capacity = next_power_of_two(slot_count);
  if(vt_capacity == slot_count)
    vt_capacity = next_power_of_two(vt_capacity+1);
  const int vtable_with_slots_size = sizeof(struct ft_vtable) + sizeof(struct ft_vtable_slot) * vt_capacity;

  int static_parents_count = 0;
  for(int i = 0; i < slot_count; ++i)
  {
    struct ft_vtable_slot_desc* slot = slots+i;
    if((slot->type & ftvs_static_parent) == ftvs_static_parent)
      ++static_parents_count;
  }

  const int vtable_full_size = vtable_with_slots_size + sizeof(oop) * static_parents_count;
  struct ft_vtable* vt = (struct ft_vtable*)ft_allocator_alloc(allocator, vtable, vtable_full_size);
  assert(vt);
  vt->instance_size = base_size;
  vt->capacity_mask = vt_capacity - 1;
  vt->parents_count = 0;
  vt->slots_begin = base_size;
  vt->slots_count = 0;
  vt->static_parents_begin = vtable_with_slots_size / sizeof(oop);
  memset(vt->slots, 0, sizeof(struct ft_vtable_slot) * vt_capacity);// vtable_with_slots_size - sizeof(struct ft_vtable_slot));

  struct vt_tmp_data_slot data_slots[slot_count];
  struct vt_tmp_data_slot* data_slot_ptr = data_slots;
  struct vt_tmp_setter_slot setter_slots[slot_count];
  struct vt_tmp_setter_slot* setter_slot_ptr = setter_slots;

  int static_parent_counter = 0;
  for(int i = 0; i < slot_count; ++i)
  {
    struct ft_vtable_slot_desc* slot = slots+i;
    enum ft_vtable_slot_type type = slot->type;
    switch(type)
    {
    case ftvs_static_slot:
      vtable_insert(vt, type, slot->name, slot->value);
      break;

    case ftvs_static_parent:
      vtable_insert(vt, type, slot->name, INT_OOP(static_parent_counter));
      ++vt->static_parents_count;
      ft_vtable_static_parents_begin(vt)[static_parent_counter++] = slot->value;
      break;

    case ftvs_parent_slot:
    {
      int index = vt->instance_size++;
      vtable_insert(vt, type, slot->name, INT_OOP(index));
      data_slot_ptr->slot_name = slot->name;
      data_slot_ptr->index = index;
      ++data_slot_ptr;
      ++vt->parents_count;
      ++vt->slots_count;
      break;
    }

    case ftvs_data_slot:
      data_slot_ptr->slot_name = slot->name;
      data_slot_ptr->index = -1;
      ++data_slot_ptr;
      ++vt->slots_count;
      break;

    case ftvs_setter_slot:
      setter_slot_ptr->slot_name = slot->name;
      setter_slot_ptr->slot_to_set = slot->value;
      ++setter_slot_ptr;
      break;
    }

  }

  for(struct vt_tmp_data_slot* data_slot = data_slots; data_slot != data_slot_ptr; ++data_slot)
  {
    if(data_slot->index == -1)
    {
      data_slot->index = vt->instance_size++;
      vtable_insert(vt, ftvs_data_slot, data_slot->slot_name, INT_OOP(data_slot->index));
    }
  }

  for(struct vt_tmp_setter_slot* setter_slot = setter_slots; setter_slot != setter_slot_ptr; ++setter_slot)
  {
    for(struct vt_tmp_data_slot* data_slot = data_slots; data_slot != data_slot_ptr; ++data_slot)
    {
      if(ft_string_equals(data_slot->slot_name, setter_slot->slot_to_set))
      {
        ft_debug("  setter slot '%s' '%s' index = %ld", ft_string_bytes(setter_slot->slot_name), ft_string_bytes(setter_slot->slot_to_set), data_slot->index);
        vtable_insert(vt, ftvs_setter_slot, setter_slot->slot_name, INT_OOP(data_slot->index));
        goto next_setter;
      }
    }

    // TODO handle failure here
    ft_debug("failed to find slot '%s' for setter '%s'", ft_string_bytes(setter_slot->slot_to_set), ft_string_bytes(setter_slot->slot_name));

  next_setter:
    (void)0;
  }

  assert(vt->instance_size == vt->slots_begin + vt->slots_count);

  return vt;
}

struct ft_object* ft_vtable_instantiate(struct ft_allocator* allocator, struct ft_vtable* vtable)
{
  struct ft_object* new_object = (struct ft_object*)ft_allocator_alloc(allocator, vtable, sizeof(oop) * vtable->instance_size);
  return new_object;
}




oop ft_vtable_get_code(struct ft_vtable* vt)
{
  return vt->code;
}

oop ft_vtable_set_code(struct ft_vtable* vt, oop code)
{
  return vt->code = code;
}



