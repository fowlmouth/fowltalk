#include "fowltalk.h"
#include "fowltalk/string.h"

#include <string.h>

struct ft_string
{
  int size_, hash;
  char data[0];
};

int djb2(const char* str)
{
  int h = 5381;
  for(char c = *str++; c; c = *str++)
    h = (h << 5) + h + c;
  return h;
}

struct ft_string* ft_new_string(struct ft_allocator* allocator, struct ft_vtable* vtable, int size, const char* str)
{
  struct ft_string* new_string = ft_allocator_alloc(allocator, vtable, sizeof(struct ft_string) + size + 1);
  new_string->size_ = size;
  if(str)
  {
    memcpy(new_string->data, str, size);
    new_string->data[size] = 0;
    new_string->hash = djb2(new_string->data);
  }
  return new_string;
}

const char* ft_string_bytes(struct ft_string* string)
{
  return string->data;
}

char* ft_string_bytes_begin(struct ft_string* string)
{
  return string->data;
}

char* ft_string_bytes_end(struct ft_string* string)
{
  return ft_string_bytes_begin(string) + ft_string_size(string);
}

int ft_string_compare(struct ft_string* a, struct ft_string* b)
{
  return strcmp(ft_string_bytes(a), ft_string_bytes(b));
}

int ft_string_hash(struct ft_string* string)
{
  return string->hash;
}

int ft_string_size(struct ft_string* string)
{
  return string->size_;
}
