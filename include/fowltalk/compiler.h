#ifndef H_FOWLTALK_COMPILER
#define H_FOWLTALK_COMPILER

#include "fowltalk.h"
#include "parser.h"
#include <stdio.h>


// void ft_init_parser(struct ft_parser* parser, struct ft_parser_actions* actions);

// void ft_parser_parse(struct ft_parser* p, struc, struct ft_parser_input* input);

struct ft_compiler_options
{
  oop bytecode_prototype;
  struct ft_vtable* vtable_vt;
};

void ft_init_compiler_options(struct ft_environment* env, struct ft_compiler_options* opt);


oop ft_compile(struct ft_environment* env, struct ft_compiler_options* compiler_options, struct ft_parser_input* input);
oop ft_compile_file(struct ft_environment* env, struct ft_compiler_options* compiler_options, FILE* file);
oop ft_compile_string(struct ft_environment* env, struct ft_compiler_options* compiler_options, int size, const char* string);

#endif
