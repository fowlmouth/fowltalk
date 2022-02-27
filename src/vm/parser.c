#ifndef FT_DEBUG_PARSER
#define FT_DEBUG_PARSER 0
#endif

#define FT_DEBUG FT_DEBUG_PARSER
#include "fowltalk/debug.h"
#include "fowltalk/parser.h"

#include <string.h>


void ft_parser_consume_token(struct ft_parser* parser);

void ft_init_parser(struct ft_parser* parser, struct ft_parser_actions* actions, struct ft_parser_input* input)
{
  memset(parser, 0, sizeof(struct ft_parser));
  ft_init_parser_lexer(&parser->lex, input);
  ft_parser_consume_token(parser);
  parser->actions = actions;
}

void ft_parser_expected(struct ft_parser* p, const char* message)
{
  p->error_code = ftpec_failed_expectation;
  struct ft_parser_position pos = ft_parser_get_position(p);
  int len = snprintf(p->error_message, FT_PARSER_ERROR_MESSAGE_BUFFER_SIZE-1, "expected %s at line %ld column %ld",
    message, pos.line, pos.col);
  p->error_message[len] = 0;
}


struct ft_parser_position ft_parser_get_position(struct ft_parser* p)
{
  return p->next.begin;
}

void ft_parser_set_position(struct ft_parser* p, struct ft_parser_position pos)
{
  ft_parser_lexer_set_position(&p->lex, pos);
  ft_parser_consume_token(p);
}

void ft_parser_consume_token(struct ft_parser* parser)
{
  if(!ft_parser_lexer_read_token(&parser->lex, &parser->next, FT_PARSER_ERROR_MESSAGE_BUFFER_SIZE, parser->error_message))
  {
    parser->error_code = ftpec_invalid_token;
  }
  struct ft_parser_position pos = ft_parser_get_position(parser);
  ft_debug("next token type (%d) value (%s) pos (%d)", (int)parser->next.type, parser->next.data, (int)pos.position);
}

bool ft_parser_is_token(struct ft_parser* parser, enum ft_parser_token_type type)
{
  return parser->next.type == type;
}

bool ft_parser_match_token(struct ft_parser* parser, int count, enum ft_parser_token_type* argv)
{
  enum ft_parser_token_type next = parser->next.type;
  for(int i = 0; i < count; ++i)
  {
    if(next == argv[i])
      return true;
  }
  return false;
}

bool ft_parser_is_eof(struct ft_parser* p)
{
  return ft_parser_is_token(p, ftptt_eof);
}

int ft_parser_skip_newlines(struct ft_parser* parser)
{
  int skipped = 0;
  while(ft_parser_is_token(parser, ftptt_newline))
  {
    ft_parser_consume_token(parser);
    ++skipped;
  }
  return skipped;
}


bool ft_parser_should_continue(struct ft_parser* p, void* arg)
{
  bool(*should_continue)(void*) = p->actions->should_continue;
  if(should_continue)
    return should_continue(arg);
  return true;
}


bool ft_parser_expect_token(struct ft_parser* parser, enum ft_parser_token_type type)
{
  if(ft_parser_is_token(parser, type))
  {
    return true;
  }
  return false;
}

bool ft_parser_parse_statement(struct ft_parser* p, void* arg);
bool ft_parser_parse_expression(struct ft_parser* p, void* arg);
bool ft_parser_parse_parent_assignment(struct ft_parser* p, void* arg);
bool ft_parser_parse_assignment(struct ft_parser* p, void* arg);
bool ft_parser_parse_method(struct ft_parser* p, void* arg);
bool ft_parser_skip_statement_separator(struct ft_parser* p);

bool ft_parser_parse_object_statement(struct ft_parser* p, void* arg)
{
  if(ft_parser_parse_parent_assignment(p, arg))
    return true;
  if(ft_parser_parse_method(p, arg))
    return true;

  return false;
}

bool ft_parser_parse_object(struct ft_parser* p, void* arg)
{
  if(ft_parser_is_token(p, ftptt_open_brace))
  {
    ft_parser_consume_token(p);
    p->actions->begin_object(arg);
    ft_parser_skip_newlines(p);
    while(ft_parser_parse_object_statement(p, arg))
    {
      if(!ft_parser_skip_statement_separator(p))
      {
        break;
      }
    }
    ft_parser_skip_newlines(p);
    if(!ft_parser_expect_token(p, ftptt_close_brace))
    {
      ft_debug("failed to find close_brace got '%d' '%s'", p->next.type, p->next.data);
      return false;
    }
    ft_parser_consume_token(p);
    p->actions->accept_object(arg);
    return true;
  }
  return false;
}

bool ft_parser_parse_block_statement(struct ft_parser* p, void* arg)
{
  return ft_parser_parse_statement(p, arg);
}

bool ft_parser_parse_block(struct ft_parser* p, void* arg)
{
  if(ft_parser_is_token(p, ftptt_open_bracket))
  {
    ft_parser_consume_token(p);
    char argument_names_buffer[1024];
    char* argument_names[32];
    memset(argument_names_buffer, 0, sizeof(argument_names_buffer));
    memset(argument_names, 0, sizeof(argument_names));
    int argc = 0;
    if(ft_parser_is_token(p, ftptt_pipe))
    {
      char* argument_names_buffer_ptr = argument_names_buffer;
      char* argument_names_buffer_end = argument_names_buffer + 1024;
      char** argument_names_ptr = argument_names;
      ft_parser_consume_token(p);
      while(ft_parser_is_token(p, ftptt_identifier))
      {
        *argument_names_ptr++ = argument_names_buffer_ptr;
        argument_names_buffer_ptr += 1 + snprintf(argument_names_buffer_ptr, argument_names_buffer_end - argument_names_buffer_ptr, "%s", p->next.data);
        ++argc;
        ft_parser_consume_token(p);
        if(ft_parser_is_token(p, ftptt_period))
          ft_parser_consume_token(p);
      }
      if(!ft_parser_expect_token(p, ftptt_pipe))
      {
        return false;
      }
      ft_parser_consume_token(p);
    }

    p->actions->begin_block(argc, (const char**)argument_names, arg);
    ft_parser_skip_newlines(p);

    while(ft_parser_parse_block_statement(p, arg))
    {
      if(!ft_parser_skip_statement_separator(p))
      {
        break;
      }
      p->actions->accept_statement(arg);
    }
    ft_parser_skip_newlines(p);

    if(!ft_parser_expect_token(p, ftptt_close_bracket))
    {
      return false;
    }
    ft_parser_consume_token(p);
    p->actions->accept_block(argc, (const char**)argument_names, arg);
    return true;
  }
  return false;
}

bool ft_parser_parse_terminal(struct ft_parser* p, void* arg)
{
  switch(p->next.type)
  {
  case ftptt_integer:
    p->actions->accept_integer(p->next.int_value, arg);
    ft_parser_consume_token(p);
    return true;

  case ftptt_string:
    p->actions->accept_string(p->next.data, arg);
    ft_parser_consume_token(p);
    return true;

  case ftptt_identifier:
    ft_debug("consuming identifier '%s'", p->next.data);
    p->actions->accept_identifier(p->next.data, arg);
    ft_parser_consume_token(p);
    ft_debug("next token after identifier: %s  type (%d)", p->next.data, (int) p->next.type);
    // ft_parser_consume_token(p);
    // ft_debug("2 next token after identifier: %s  type (%d)", p->next.data, (int) p->next.type);
    return true;

  case ftptt_open_brace:
    return ft_parser_parse_object(p, arg);

  case ftptt_open_bracket:
    return ft_parser_parse_block(p, arg);

  case ftptt_open_paren:
    ft_parser_consume_token(p);
    ft_parser_skip_newlines(p);
    if(!ft_parser_parse_expression(p, arg))
    {
      ft_parser_expected(p, "expression");
      return false;
    }
    ft_parser_skip_newlines(p);
    if(!ft_parser_is_token(p, ftptt_close_paren))
    {
      ft_parser_expected(p, "close paren");
      return false;
    }
    ft_parser_consume_token(p);
    return true;

  default:
    return false;
  }
}

bool ft_parser_parse_unary_message(struct ft_parser* p, void* arg)
{
  if(!ft_parser_parse_terminal(p, arg))
    return false;

  while(ft_parser_is_token(p, ftptt_identifier))
  {
    p->actions->accept_send(p->next.data, 1, arg);
    ft_parser_consume_token(p);
  }
  return true;
}

bool ft_parser_parse_binary_message(struct ft_parser* p, void* arg)
{
  return ft_parser_parse_unary_message(p, arg);

  // bool have_receiver = f
  // if(!ft_parser_parse_terminal(p, arg))
  // {

  // }
}


bool ft_parser_parse_keyword_message(struct ft_parser* p, void* arg)
{
  bool have_receiver = ft_parser_parse_binary_message(p, arg);
  enum ft_parser_token_type non_expression_token_types[] = {
    ftptt_period, ftptt_close_bracket, ftptt_close_paren, ftptt_close_brace,
    ftptt_eof
  };
  if(have_receiver)
  {
    ft_parser_skip_newlines(p);
  }
  if(ft_parser_is_token(p, ftptt_keyword))
  {
    char selector_data[FT_PARSER_SELECTOR_MAX_SIZE];
    char* selector = selector_data;
    int argc = 1;
    for(; ft_parser_is_token(p, ftptt_keyword); ++argc)
    {
      int len = strlen(p->next.data);
      memcpy(selector, p->next.data, len);
      selector += len;
      ft_parser_consume_token(p);
      ft_parser_skip_newlines(p);
      if(!ft_parser_parse_binary_message(p, arg))
      {
        ft_parser_expected(p, "terminal expression");
        return false;
      }
      ft_parser_skip_newlines(p);
      if(!ft_parser_is_token(p, ftptt_keyword) && !ft_parser_match_token(p, sizeof(non_expression_token_types) / sizeof(non_expression_token_types[0]), non_expression_token_types))
      {
        ft_parser_expected(p, "keyword-expression");
        return false;
      }
    }
    *selector++ = 0;
    if(have_receiver)
      p->actions->accept_send(selector_data, argc, arg);
    else
      p->actions->accept_self_send(selector_data, argc-1, arg);

    return true;
  }
  else if(have_receiver && !ft_parser_match_token(p, sizeof(non_expression_token_types) / sizeof(non_expression_token_types[0]), non_expression_token_types))
  {
    ft_parser_expected(p, "keyword-expression");
    return false;
  }
  return have_receiver;
}


bool ft_parser_parse_expression(struct ft_parser* p, void* arg)
{
  return ft_parser_parse_keyword_message(p, arg);
}

int ft_parser_token_str(struct ft_parser* p, const char* str)
{
  return strcmp(p->next.data, str);
}

bool ft_parser_parse_parent_assignment(struct ft_parser* p, void* arg)
{
  if(ft_parser_is_token(p, ftptt_identifier))
  {
    struct ft_parser_position start = ft_parser_get_position(p);
    int is_parent = 0;
    char name[FT_PARSER_TOKEN_SIZE];
    strcpy(name, p->next.data);
    ft_parser_consume_token(p);
    if(ft_parser_is_token(p, ftptt_operator) && !ft_parser_token_str(p, "*"))
    {
      ft_parser_consume_token(p);
      is_parent = 1;
    }
    if(ft_parser_is_token(p, ftptt_assignment))
    {
      char operator[FT_PARSER_TOKEN_SIZE];
      strcpy(operator, p->next.data);
      ft_parser_consume_token(p);
      if(ft_parser_parse_expression(p, arg))
      {
        p->actions->accept_assignment(name, operator, arg, is_parent);
        return true;
      }
      else
      {
        ft_parser_expected(p, "expression");
      }
    }
    ft_parser_set_position(p, start);
  }
  return false;
}

bool ft_parser_parse_assignment(struct ft_parser* p, void* arg)
{
  if(ft_parser_is_token(p, ftptt_identifier))
  {
    struct ft_parser_position start = ft_parser_get_position(p);
    ft_debug("entered parse_assignment at (line=%ld col=%ld pos=%ld)", start.line, start.col, start.position);
    char name[FT_PARSER_TOKEN_SIZE];
    strcpy(name, p->next.data);
    ft_parser_consume_token(p);
    if(ft_parser_is_token(p, ftptt_assignment))
    {
      char operator[FT_PARSER_TOKEN_SIZE];
      strcpy(operator, p->next.data);
      ft_parser_consume_token(p);
      if(ft_parser_parse_expression(p, arg))
      {
        p->actions->accept_assignment(name, operator, arg, 0);
        return true;
      }
      else
      {
        ft_parser_expected(p, "expression");
      }
    }
    ft_parser_set_position(p, start);

    struct ft_parser_position pos = ft_parser_get_position(p);
    ft_debug("aborting parse_assignment at (line=%ld col=%ld pos=%ld)", pos.line, pos.col, pos.position);
  }
  return false;
}

const char* ft_parser_token_data(struct ft_parser* p)
{
  return p->next.data;
}

bool ft_parser_parse_method(struct ft_parser* p, void* arg)
{
  struct ft_parser_position start = ft_parser_get_position(p);
  char selector[FT_PARSER_SELECTOR_MAX_SIZE];
  int argc = 1;
  char argument_names_buffer[1024];
  const char* arg_names[FT_PARSER_MAX_MESSAGE_ARGUMENTS];
  arg_names[0] = "self";

  if(ft_parser_is_token(p, ftptt_identifier))
  {
    memcpy(selector, p->next.data, strlen(p->next.data)+1);
    ft_parser_consume_token(p);
  }
  else if(ft_parser_is_token(p, ftptt_keyword))
  {
    char* selector_str = selector;
    char* argument_names_buffer_ptr = argument_names_buffer;
    const char** arg_names_ptr = arg_names+1;
    do
    {
      int len = strlen(p->next.data);
      memcpy(selector_str, p->next.data, len);
      selector_str += len;
      ft_parser_consume_token(p);
      if(!ft_parser_is_token(p, ftptt_identifier))
      {
        goto rewind;
      }
      len = strlen(ft_parser_token_data(p));
      memcpy(argument_names_buffer_ptr, ft_parser_token_data(p), len);
      argument_names_buffer_ptr[len] = 0;
      *arg_names_ptr++ = argument_names_buffer_ptr;
      argument_names_buffer_ptr += len + 1;
      ft_parser_consume_token(p);
      ++argc;

    } while(ft_parser_is_token(p, ftptt_keyword));

    *selector_str++ = 0;

  }
  else
  {
    return false;
  }

  if(!ft_parser_is_token(p, ftptt_open_bracket))
  {
    goto rewind;
  }
  ft_parser_consume_token(p);

  ft_debug("parsing method selector= '%s' argc= %d", selector, argc);

  // {
  //   const char** arg_names_ptr = arg_names;
  //   const char* argument_names_buffer_ptr = argument_names_buffer;
  //   *arg_names_ptr++ = "self";
  //   for(int i = argc; i--;)
  //   {
  //     *arg_names_ptr++ = argument_names_buffer_ptr;
  //     int len = strlen(argument_names_buffer_ptr);
  //     argument_names_buffer_ptr += len + 1;
  //   }
  // }

  p->actions->begin_method(selector, argc, arg_names, arg);

  ft_parser_skip_newlines(p);

  while(ft_parser_parse_statement(p, arg))
  {
    p->actions->accept_statement(arg);
    if(!ft_parser_skip_statement_separator(p))
    {
      break;
    }
    if(!ft_parser_should_continue(p, arg))
    {
      ft_debug("exiting because shouldn't continue");
      break;
    }
  }

  ft_parser_skip_newlines(p);
  if(!ft_parser_is_token(p, ftptt_close_bracket))
  {
    ft_debug("exiting failed to find close bracket");
    ft_parser_expected(p, "]");
    // TODO implement parser_actions.aborted_method for this case
    goto rewind;
  }
  ft_parser_consume_token(p);

  p->actions->accept_method(selector, argc, arg_names, arg);
  return true;

rewind:
  ft_parser_set_position(p, start);
  return false;
}

bool ft_parser_parse_return(struct ft_parser* p, void* arg)
{
  if(ft_parser_is_token(p, ftptt_caret))
  {
    ft_parser_consume_token(p);
    if(!ft_parser_parse_expression(p, arg))
    {
      ft_parser_expected(p, "expression");
      return false;
    }

    p->actions->accept_return(arg);
    return true;
  }
  return false;
}

bool ft_parser_parse_statement(struct ft_parser* p, void* arg)
{
  if(ft_parser_parse_assignment(p, arg))
    return true;
  if(ft_parser_parse_return(p, arg))
    return true;
  return ft_parser_parse_expression(p, arg);
}

bool ft_parser_skip_statement_separator(struct ft_parser* p)
{
  struct ft_parser_position start = ft_parser_get_position(p);
  ft_parser_skip_newlines(p);
  if(ft_parser_is_token(p, ftptt_period))
  {
    ft_parser_consume_token(p);
    ft_parser_skip_newlines(p);
    return true;
  }
  ft_parser_set_position(p, start);
  return false;
}

void ft_parser_parse_script(struct ft_parser* p, void* arg)
{
  ft_parser_skip_newlines(p);
  while(!p->error_code && ft_parser_parse_statement(p, arg))
  {
    p->actions->accept_statement(arg);

    if(!ft_parser_skip_statement_separator(p))
    {
      break;
    }
    if(!ft_parser_should_continue(p, arg))
    {
      ft_debug("exiting because of parser should continue returning false");
      break;
    }
  }
}
