#include "gc.h"



struct gc_header* gc_pointer_header(void* ptr)
{
  return (struct gc_header*)ptr - 1;
}

int gc_init(struct gc* gc, int size, int flags)
{
  void* mem = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);
  if(mem == (void*)-1)
  {
    fprintf(stderr, "mmap failed\n");
    gc->size = 0;
    gc->flags = 0;
    gc->mem = NULL;
    gc->next = NULL;
    return 1;
  }
  if(pthread_mutex_init(&gc->mutex, NULL))
  {
    fprintf(stderr, "mutex init failed\n");
    munmap(mem, size);
    return 2;
  }
  gc->size = size;
  gc->flags = flags;
  gc->mem = mem;
  gc->next = (char*)mem;
  gc->allocation_count = 0;
  gc->root_count = 0;
  gc->root_capacity = 0;
  gc->roots = NULL;
  return 0;
}

void gc_deinit(struct gc* gc)
{
  pthread_mutex_destroy(&gc->mutex);
  free(gc->roots);
}

void gc_grow_root_capacity(struct gc* gc, int new_capacity)
{
  if(new_capacity <= gc->root_capacity)
  {
    return;
  }

  struct gc_root* new_roots = (struct gc_root*)calloc(sizeof(struct gc_root), new_capacity);
  memcpy(new_roots, gc->roots, sizeof(struct gc_root) * gc->root_capacity);
  free(gc->roots);
  gc->roots = new_roots;
  gc->root_capacity = new_capacity;
}

void gc_grow_if_needed(struct gc* gc)
{
  if(gc->root_count < gc->root_capacity)
  {
    gc_grow_root_capacity(gc, gc->root_capacity ? gc->root_capacity * 2 : 8);
  }
}

void gc_add_roots(struct gc* gc, void** new_roots, int count)
{
  gc_grow_if_needed(gc);
  struct gc_root* gc_root = gc->roots + gc->root_count++;
  gc_root->roots = new_roots;
  gc_root->count = count;
}

int gc_find_roots(struct gc* gc, void** roots)
{
  for(int i = 0; i < gc->root_count; ++i)
  {
    if(gc->roots[i].roots == roots)
      return i;
  }
  return -1;
}

void gc_remove_roots(struct gc* gc, void** remove_roots)
{
  int index = gc_find_roots(gc, remove_roots);
  if(index == -1)
    return;

  for(; index < gc->root_count-1; ++index)
  {
    gc->roots[index] = gc->roots[index+1];
  }
  --gc->root_count;
}

void* gc_alloc(struct gc* gc, int size, int flags)
{
  int aligned_size = ALIGN8(size);

  pthread_mutex_lock(&gc->mutex);
  struct gc_header* header = (struct gc_header*)gc->next;
  gc->next += sizeof(struct gc_header) + aligned_size;
  ++gc->allocation_count;
  pthread_mutex_unlock(&gc->mutex);

  header->size_words = aligned_size / sizeof(void*);
  header->flags = flags;
  header->vtable = NULL;

  void* object = (void*)(header+1);

  return object;
}

bool gc_includes_object(struct gc* gc, void* object)
{
  return object >= gc->mem && object <= (void*)((const char*)gc->mem + gc->size);
}

void gc_mark_object(struct gc* gc, void* object);

void gc_mark(struct gc* gc, struct gc_header* header)
{
  if(header->flags & gcof_marked)
  {
    return;
  }
  header->flags |= gcof_marked;
  if((header->flags & gcof_bytes) == 0)
  {
    void** object = (void**)(header + 1);
    for(int i = 0; i < header->size_words; ++i)
    {
      gc_mark_object(gc, object[i]);
    }
  }
}

#include <stdio.h>

void gc_collect(struct gc* gc)
{
  for(struct gc_header* hdr = (struct gc_header*)gc->mem;
    (void*)hdr < (void*)gc->next;
    hdr = (struct gc_header*)((const char*)hdr + sizeof(struct gc_header) + (sizeof(void*) * hdr->size_words)))
  {
    hdr->flags = hdr->flags & ~gcof_marked;
  }

  for(int i = 0; i < gc->root_count; ++i)
  {
    struct gc_root* root = gc->roots + i;

    for(void **iter = root->roots, **end = root->roots + root->count;
      iter < end;
      ++iter)
    {
      gc_mark_object(gc, *iter);
    }
  }

  int marked_count = 0;
  for(struct gc_header* hdr = (struct gc_header*)gc->mem;
    (void*)hdr < (void*)gc->next;
    hdr = (struct gc_header*)((const char*)hdr + sizeof(struct gc_header) + (sizeof(void*) * hdr->size_words)))
  {
    if(hdr->flags & gcof_marked)
      ++marked_count;
  }
  printf("marked count: %d\n", marked_count);
}


