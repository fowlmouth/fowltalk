#ifndef H_FOWLTALK_ARRAY
#define H_FOWLTALK_ARRAY

struct ft_array;

struct ft_array* ft_new_array(struct ft_allocator* allocator, struct ft_vtable* vtable, int size);

int ft_array_size(struct ft_array*);
oop* ft_array_items_begin(struct ft_array*);
oop* ft_array_items_end(struct ft_array*);

#endif
