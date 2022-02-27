#ifndef H_FOWLTALK_STRING
#define H_FOWLTALK_STRING

struct ft_string;

int djb2(const char* str);
struct ft_string* ft_new_string(struct ft_allocator* allocator, struct ft_vtable* vtable, int size, const char* str);
const char* ft_string_bytes(struct ft_string* string);
int ft_string_compare(struct ft_string* a, struct ft_string* b);
int ft_string_hash(struct ft_string* string);
int ft_string_size(struct ft_string* string);

char* ft_string_bytes_begin(struct ft_string* string);
char* ft_string_bytes_end(struct ft_string* string);

#endif
