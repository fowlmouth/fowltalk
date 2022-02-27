#ifndef H_FOWLTALK_BYTES
#define H_FOWLTALK_BYTES

#include "fowltalk.h"

typedef uint8_t ft_byte_t;

struct ft_bytes;

struct ft_bytes* ft_new_bytes(struct ft_allocator* allocator, struct ft_vtable* vtable, int size);
int ft_bytes_size(struct ft_bytes*);
ft_byte_t* ft_bytes_data_begin(struct ft_bytes* bytes);
ft_byte_t* ft_bytes_data_end(struct ft_bytes* bytes);

#endif
