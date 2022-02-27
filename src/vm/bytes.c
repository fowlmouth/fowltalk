
#include "fowltalk.h"
#include "fowltalk/bytes.h"

typedef uint8_t ft_byte_t;

struct ft_bytes
{
  int size;
  ft_byte_t data[0];
};

struct ft_bytes* ft_new_bytes(struct ft_allocator* allocator, struct ft_vtable* vtable, int size)
{
  struct ft_bytes* bytes = (struct ft_bytes*)ft_allocator_alloc(allocator, vtable, sizeof(struct ft_bytes) + size);
  bytes->size = size;
  return bytes;
}

int ft_bytes_size(struct ft_bytes* bytes)
{
  return bytes->size;
}

ft_byte_t* ft_bytes_data_begin(struct ft_bytes* bytes)
{
  return bytes->data;
}

ft_byte_t* ft_bytes_data_end(struct ft_bytes* bytes)
{
  return bytes->data + bytes->size;
}
