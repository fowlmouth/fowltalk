#ifndef H_FOWLTALK_GC
#define H_FOWLTALK_GC

#include <pthread.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

enum gc_object_flags
{
  gcof_deleted = 1 << 0,
  gcof_marked  = 1 << 1,
  gcof_bytes   = 1 << 2,
  gcof__bits = 3,
  gcof__mask = (1 << gcof__bits) - 1
};


struct gc_header
{
  int size_words, flags;
  struct ft_vtable* vtable;
};

#define ALIGN8(num) (((num) + 7) & ~7)

struct gc_root
{
  void** roots;
  int count;
};

struct gc
{
  int size, flags;
  void* mem;
  char* next;
  int allocation_count;

  int root_count, root_capacity;
  struct gc_root* roots;
  pthread_mutex_t mutex;
};


int gc_init(struct gc* gc, int size, int flags);
void gc_deinit(struct gc* gc);

void gc_add_roots(struct gc* gc, void** new_roots, int count);
int gc_find_roots(struct gc* gc, void** roots);
void gc_remove_roots(struct gc* gc, void** remove_roots);
void* gc_alloc(struct gc* gc, int size, int flags);

void gc_mark(struct gc* gc, struct gc_header* header);

void gc_collect(struct gc* gc);

#endif
