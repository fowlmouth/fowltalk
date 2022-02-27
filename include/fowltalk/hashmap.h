#ifndef H_FOWLTALK_HASHMAP
#define H_FOWLTALK_HASHMAP

struct ft_hashmap;

struct ft_environment* ft_new_environment();
struct ft_allocator* ft_environment_allocator(struct ft_environment*);

oop ft_environment_namespace(struct ft_environment*);
struct ft_string* ft_environment_intern(struct ft_environment*, const char*);

#endif
