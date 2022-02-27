#ifndef H_FOWLTALK_ENVIRONMENT
#define H_FOWLTALK_ENVIRONMENT

#include "fowltalk/primitives.h"

struct ft_environment;

struct ft_environment_primitive_method
{
  struct ft_string* selector;
  ft_primitive_t function;
};



struct ft_environment* ft_new_environment(const char* dir, const char* platform_name);
struct ft_environment* ft_new_minimal_environment();
struct ft_allocator* ft_environment_allocator(struct ft_environment*);


ft_primitive_t ft_environment_primitive(struct ft_environment* env, struct ft_string* selector);
void ft_environment_set_primitives(struct ft_environment* env, int primitive_count, struct ft_environment_primitive_method* methods);

oop ft_environment_lobby(struct ft_environment* env);
struct ft_string* ft_environment_intern(struct ft_environment*, const char*);

#endif
