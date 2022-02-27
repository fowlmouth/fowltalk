#ifndef H_FOWLTALK
#define H_FOWLTALK

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef void *oop;
typedef intptr_t ft_integer_t;

#define OOP_IS_NIL(val) (!(val))
#define OOP_IS_INT(val) ((ft_integer_t)(val) & 1)
#define OOP_IS_PTR(val) ((val) && !OOP_IS_INT(val))
#define OOP_INT(val) ((ft_integer_t)(val) >> 1)
#define INT_OOP(val) ((oop)(((ft_integer_t)(val) << 1) | 1))
#define OOP_NIL ((oop)NULL)


struct ft_vtable;


struct ft_object_header;

struct ft_object_header* ft_object_header(oop value);
struct ft_vtable* ft_vtable(oop value);
void ft_set_vtable(oop object, struct ft_vtable* vt);


struct ft_environment;
struct ft_allocator;

void ft_allocator_debug_print(struct ft_allocator* allocator);
void* ft_allocator_alloc(struct ft_allocator* allocator, struct ft_vtable* vtable, int bytes);
void ft_allocator_free(struct ft_allocator* allocator, struct ft_object_header* obj);


struct ft_string;

typedef oop* ft_object;



enum ft_lookup_result
{
  ftlr_not_found, ftlr_data, ftlr_setter
};

enum ft_lookup_result ft_lookup(oop object, struct ft_string* selector, oop* context, oop* result, ft_integer_t* setter_index);
oop ft_simple_lookup(oop object, struct ft_string* selector);

oop ft_perform0(oop receiver, struct ft_string* selector);

struct ft_string* ft_new_string(struct ft_allocator* allocator, struct ft_vtable* vtable, int size, const char* str);

oop ft_init(struct ft_environment* env, const char*, const char*);

oop ft_apply(oop method, oop receiver, int argc, oop* argv);

oop ft_clone(struct ft_allocator* allocator, oop value);
oop ft_deep_clone(struct ft_allocator* allocator, oop value, int depth);

#endif
