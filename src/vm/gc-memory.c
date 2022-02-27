#define FT_DEBUG FT_DEBUG_MEMORY
#include "fowltalk/debug.h"

#include "gc.h"
#include "object-header.h"
#include "fowltalk/environment.h"
#include "memory.h"
#include "fowltalk/vtable.h"

#define ALIGN8(num) (((num) + 7) & ~7)

bool ft_init_allocator(struct ft_allocator* allocator, int memory_size)
{
  gc_init(&allocator->gc, memory_size, 0);
  return allocator->gc.mem;
}

void* ft_allocator_alloc(struct ft_allocator* allocator, struct ft_vtable* vtable, int bytes)
{
  void* p = gc_alloc(&allocator->gc, bytes, 0);
  ft_set_vtable(p, vtable);
  ft_debug("allocated %d bytes at %p vtable= %p", bytes, p, ft_object_vtable(p));
  return p;
}

void ft_set_vtable(oop object, struct ft_vtable* vt)
{
  ft_object_header(object)->base.vtable = vt;
}


void ft_allocator_debug_print(struct ft_allocator* allocator)
{
  // ft_debug("used:");
  // for(struct ft_object_header* obj = allocator->used; obj; obj = obj->next)
  // {
  //   ft_debug("  %p words=%d", obj, obj->size_words);
  // }

  // ft_debug("free:");
  // for(struct ft_object_header* obj = allocator->free; obj; obj = obj->next)
  // {
  //   ft_debug("  %p words=%d", obj, obj->size_words);
  // }
}


void ft_allocator_each_object(struct ft_allocator* allocator, ft_object_predicate_fn pred, ft_object_callback callback, void* arg)
{
  for(struct gc_header* header = allocator->gc.mem;
    (void*)header < (void*)allocator->gc.next;
    header = (struct gc_header*)((char*)(header + 1) + (header->size_words * sizeof(void*))))
  {
    void* obj = (void*)(header + 1);
    if(pred(obj))
    {
      callback(obj, arg);
    }
  }
}



