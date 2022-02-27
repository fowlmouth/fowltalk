#ifndef H_FOWLTALK_PARSER_INPUT
#define H_FOWLTALK_PARSER_INPUT

#include <stdio.h>
#include <stdbool.h>

#define FT_PARSER_EOF_VALUE -1

typedef long int ft_parser_input_position_t;

struct ft_parser_input_vtable;

struct ft_parser_input
{
  struct ft_parser_input_vtable* vt;
};

struct ft_parser_input_vtable
{
  int (*next_char)(struct ft_parser_input*);
  ft_parser_input_position_t (*get_position)(struct ft_parser_input*);
  void (*set_position)(struct ft_parser_input*, ft_parser_input_position_t position);
  bool (*is_eof)(struct ft_parser_input*);
};


struct ft_parser_file_input
{
  struct ft_parser_input base;
  FILE* file;
};

struct ft_parser_string_input
{
  struct ft_parser_input base;
  const char* string;
  ft_parser_input_position_t position;
  int string_size;
};


void ft_init_parser_file_input(struct ft_parser_file_input*, FILE*);
void ft_init_parser_string_input(struct ft_parser_string_input*, int, const char*);

struct ft_parser_input_vtable* ft_parser_input_vtable(struct ft_parser_input* input);


#endif
