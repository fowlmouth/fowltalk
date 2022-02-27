#ifndef H_FOWLTALK_PRIMITIVES
#define H_FOWLTALK_PRIMITIVES

#include "vm.h"

typedef void(*ft_primitive_t)(struct ft_virtual_machine*, struct ft_environment*, int, oop*);

void ft_primitives_init(struct ft_environment* env);

#endif
