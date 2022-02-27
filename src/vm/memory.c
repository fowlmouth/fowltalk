#define FT_DEBUG 0

#include "fowltalk.h"
#include "fowltalk/debug.h"
#include "object-header.h"
#include <sys/mman.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#define ALIGN8(num) (((num) + 7) & ~7)

bool ft_init_allocator(struct ft_allocator* allocator, int memory_size)
{
  allocator->free = NULL;
  allocator->used = NULL;
  allocator->start = NULL;
  allocator->end = NULL;

  void* start = mmap(0, memory_size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, 0, 0);
  if(start == (void*)-1)
  {
    fprintf(stderr, "error mapping memory\n");
    return false;
  }

  memset(start, 0, memory_size);

  allocator->free = (struct ft_object_header*)start;
  allocator->free->size_words = (memory_size - sizeof(struct ft_object_header)) / sizeof(oop);
  allocator->free->flags = ftohf_deleted;

  allocator->start = start;
  allocator->end = start+memory_size;
  return true;
}

void* ft_allocator_alloc(struct ft_allocator* allocator, struct ft_vtable* vtable, int bytes)
{
  bytes = ALIGN8(bytes);
  int words = bytes / sizeof(oop);
  int smallest_size = INT_MAX;
  struct ft_object_header** smallest = NULL;
  struct ft_object_header* selected = NULL;
  for(struct ft_object_header** p = &allocator->free; *p; p = &((*p)->next))
  {
    int size = (*p)->size_words;
    if(size == words)
    {
      selected = *p;
      *p = (*p)->next;
      goto have_selected;
    }
    if(size > words && size < smallest_size)
    {
      smallest = p;
      smallest_size = size;
    }
  }

  if(smallest)
  {
    const int header_words = sizeof(struct ft_object_header) / sizeof(oop);
    const int minimum_size_for_splitting = words + header_words + 8;
    if((*smallest)->size_words > minimum_size_for_splitting)
    {
      selected = *smallest;
      *smallest = (struct ft_object_header*)((oop*)(*smallest + 1) + words);
      (*smallest)->size_words = selected->size_words - header_words - words;
      (*smallest)->next = selected->next;
      (*smallest)->flags = ftohf_deleted;

      selected->size_words = words;
    }
    else
    {
      selected = *smallest;
      *smallest = (*smallest)->next;
    }
  }

have_selected:

  if(!selected)
    return NULL;

  selected->flags = 0;
  selected->vtable = vtable;
  selected->next = allocator->used;
  allocator->used = selected;

  ft_debug("allocated %d bytes at %p", bytes, selected);

  return selected+1;

  // struct ft_object_header* header = (struct ft_object_header*)calloc(1, bytes + sizeof(struct ft_object_header));
  // header->size_bytes = bytes;
  // header->vtable = vtable;

  // header->next = mem->used;
  // mem->used = header;

  // return header+1;
}



void _ft_allocator_add_to_free_list(struct ft_allocator* allocator, struct ft_object_header* obj)
{
  struct ft_object_header** p = &allocator->free;
  for(; *p && *p < obj; p = &(*p)->next)
    ;

  if(!(*p))
  {
    obj->next = NULL;
    *p = obj;
    return;
  }
  if(*p > obj)
  {
    ft_debug("*p= %p\nobj= %p", *p, obj);
    if(*p == (void*)((oop*)(obj+1) + obj->size_words))
    {
      // join p and obj
      obj->size_words += sizeof(struct ft_object_header) / sizeof(oop) + (*p)->size_words;
      obj->next = (*p)->next;
      *p = obj;
    }
    else
    {
      obj->next = *p;
      *p = obj;
    }
    return;
  }
  assert(false);
}


void ft_allocator_free(struct ft_allocator* allocator, struct ft_object_header* obj)
{
  struct ft_object_header** used = &allocator->used;
  for(; *used && *used != obj; used = &(*used)->next)
    ;

  if(*used == obj)
  {
    *used = obj->next;
  }
  else
  {
    return;
  }

  _ft_allocator_add_to_free_list(allocator, obj);
}


void ft_allocator_debug_print(struct ft_allocator* allocator)
{
  ft_debug("used:");
  for(struct ft_object_header* obj = allocator->used; obj; obj = obj->next)
  {
    ft_debug("  %p words=%d", obj, obj->size_words);
  }

  ft_debug("free:");
  for(struct ft_object_header* obj = allocator->free; obj; obj = obj->next)
  {
    ft_debug("  %p words=%d", obj, obj->size_words);
  }
}


