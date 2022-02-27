#include "fowltalk.h"
#include "fowltalk/parser.h"
#include "fowltalk/parser-validator.h"

bool pv_should_continue(void* arg)
{
  return 1;
}

void pv_accept_send(const char* selector, int argc, void* arg)
{
}


void pv_accept_self_send(const char* selector, int argc, void* arg)
{
}
void pv_accept_integer(ft_integer_t num, void* arg)
{
}
void pv_accept_string(const char* str, void* arg)
{
}
void pv_accept_assignment(const char* name, const char* op, void* arg, int is_parent)
{
}
void pv_accept_identifier(const char* name, void* arg)
{
}
void pv_accept_return(void* arg)
{
}
void pv_accept_statement(void* arg)
{
}
void pv_begin_object(void* arg)
{
}
void pv_accept_object(void* arg)
{
}
void pv_begin_method(const char* selector, int argc, const char** argv, void* arg)
{
}
void pv_accept_method(const char* selector, int argc, const char** argv, void* arg)
{
}
void pv_begin_block(int argc, const char** argv, void* arg)
{
}
void pv_accept_block(int argc, const char** argv, void* arg)
{
}



struct ft_parser_actions parser_actions = {
  pv_should_continue,
  pv_accept_send,
  pv_accept_self_send,
  pv_accept_integer,
  pv_accept_string,
  pv_accept_assignment,
  pv_accept_identifier,
  pv_accept_return,
  pv_accept_statement,
  pv_begin_object,
  pv_accept_object,
  pv_begin_method,
  pv_accept_method,
  pv_begin_block,
  pv_accept_block
};


bool ft_validate_source(struct ft_parser_input* input)
{
  struct ft_parser parser;
  ft_init_parser(&parser, &parser_actions, input);
  ft_parser_parse_script(&parser, NULL);
  if(ft_parser_is_eof(&parser))
  {
    return true;
  }
  return false;
}