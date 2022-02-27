#include "fowltalk/lexer.h"

int ftpif_next_char(struct ft_parser_input* input)
{
  char data;
  int read = fread(&data, 1, 1, ((struct ft_parser_file_input*)input)->file);
  if(read != 1)
  {
    return FT_PARSER_EOF_VALUE;
  }
  return data;
}


ft_parser_input_position_t ftpif_get_position(struct ft_parser_input* input)
{
  return (ft_parser_input_position_t)ftell(((struct ft_parser_file_input*)input)->file);
}

void ftpif_set_position(struct ft_parser_input* input, ft_parser_input_position_t position)
{
  fseek(((struct ft_parser_file_input*)input)->file, position, SEEK_SET);
}

bool ftpif_is_eof(struct ft_parser_input* input)
{
  return feof(((struct ft_parser_file_input*)input)->file);
}

struct ft_parser_input_vtable ftpi_file_vt = {
  ftpif_next_char,
  ftpif_get_position,
  ftpif_set_position,
  ftpif_is_eof
};

void ft_init_parser_file_input(struct ft_parser_file_input* input, FILE* file)
{
  input->base.vt = &ftpi_file_vt;
  input->file = file;
}





int ftpis_next_char(struct ft_parser_input* input)
{
  struct ft_parser_string_input* str = (struct ft_parser_string_input*)input;
  if(str->position >= str->string_size)
    return FT_PARSER_EOF_VALUE;
  return str->string[str->position++];
}


ft_parser_input_position_t ftpis_get_position(struct ft_parser_input* input)
{
  return ((struct ft_parser_string_input*)input)->position;
}

void ftpis_set_position(struct ft_parser_input* input, ft_parser_input_position_t position)
{
  ((struct ft_parser_string_input*)input)->position = position;
}

bool ftpis_is_eof(struct ft_parser_input* input)
{
  return ((struct ft_parser_string_input*)input)->position >= ((struct ft_parser_string_input*)input)->string_size;
}

struct ft_parser_input_vtable ftpi_string_vt = {
  ftpis_next_char,
  ftpis_get_position,
  ftpis_set_position,
  ftpis_is_eof
};

void ft_init_parser_string_input(struct ft_parser_string_input* input, int size, const char* string)
{
  input->base.vt = &ftpi_string_vt;
  input->string_size = size;
  input->string = string;
  input->position = 0;
}



struct ft_parser_input_vtable* ft_parser_input_vtable(struct ft_parser_input* input)
{
  return input->vt;
}

