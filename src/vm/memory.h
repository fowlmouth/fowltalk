#ifndef H_FOWLTALK_MEMORY
#define H_FOWLTALK_MEMORY

#include "fowltalk.h"

bool ft_init_allocator(struct ft_allocator* allocator, int memory_size);


typedef int(*ft_object_predicate_fn)(oop);
typedef void(*ft_object_callback)(oop, void*);
void ft_allocator_each_object(struct ft_allocator* allocator, ft_object_predicate_fn pred, ft_object_callback callback, void* arg);


#endif
