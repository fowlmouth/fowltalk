#include "fowltalk.h"
#include "fowltalk/array.h"

#include <string.h>

struct ft_array
{
  int size;
  oop items[0];
};

struct ft_array* ft_new_array(struct ft_allocator* allocator, struct ft_vtable* vtable, int size)
{
  if(size < 0)
    size = 0;
  struct ft_array* array = (struct ft_array*)ft_allocator_alloc(allocator, vtable, sizeof(struct ft_array) + sizeof(oop) * size);
  array->size = size;
  memset(array->items, 0, sizeof(oop) * size);
  return array;
}

int ft_array_size(struct ft_array* array)
{
  return array->size;
}

oop* ft_array_items_begin(struct ft_array* array)
{
  return array->items;
}

oop* ft_array_items_end(struct ft_array* array)
{
  return array->items + array->size;
}
