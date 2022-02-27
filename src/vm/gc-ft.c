#include "gc.h"
#include "fowltalk.h"
#include "object-header.h"

struct ft_object_header* ft_object_header(oop value)
{
  return (struct ft_object_header*)value - 1;
}

struct ft_vtable* ft_object_vtable(oop value)
{
  return ft_object_header(value)->base.vtable;
}

int ft_object_actual_size_bytes(oop object)
{
  return ft_object_header(object)->base.size_words * sizeof(void*);
}

void gc_mark_object(struct gc* gc, void* object)
{
  if(OOP_IS_PTR(object))
  {
    gc_mark(gc, &ft_object_header(ft_object_vtable(object))->base);
    gc_mark(gc, &ft_object_header(object)->base);
  }
}