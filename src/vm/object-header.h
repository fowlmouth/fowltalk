#ifndef H_FOWLTALK_OBJECT_HEADER
#define H_FOWLTALK_OBJECT_HEADER

#include "gc.h"
#include "fowltalk.h"

enum ft_object_header_flags
{
  ftohf_deleted = 1
};

struct ft_object_header
{
  // struct ft_object_header* next;
  // struct ft_vtable* vtable;
  // int size_words, flags;
  struct gc_header base;
};

struct ft_allocator
{
  // struct ft_object_header *free, *used;
  // void *start, *end;
  struct gc gc;
};

int ft_object_actual_size_bytes(oop object);

#endif
