#ifndef H_FOWLTALK_PARSER
#define H_FOWLTALK_PARSER

#include "lexer.h"

#define FT_PARSER_SELECTOR_MAX_SIZE 1024
#define FT_PARSER_ERROR_MESSAGE_BUFFER_SIZE 512
#define FT_PARSER_MAX_MESSAGE_ARGUMENTS 32

enum ft_parser_error_code
{
  ftpec_none = 0,
  ftpec_failed_expectation = 0x100,
  ftpec_invalid_token
};

struct ft_parser
{
  struct ft_parser_lexer lex;
  struct ft_parser_actions* actions;
  struct ft_parser_token next;
  enum ft_parser_error_code error_code;
  char error_message[FT_PARSER_ERROR_MESSAGE_BUFFER_SIZE];
};

struct ft_parser_actions
{
  bool (*should_continue)(void*);
  void (*accept_send)(const char*, int, void*);
  void (*accept_self_send)(const char*, int, void*);
  void (*accept_integer)(ft_integer_t, void*);
  void (*accept_string)(const char*, void*);
  void (*accept_assignment)(const char*, const char*, void*, int);
  // void (*accept_parent_assignment)(const char*, const char*, void*);
  void (*accept_identifier)(const char*, void*);
  void (*accept_return)(void*);
  void (*accept_statement)(void*);
  void (*begin_object)(void*);
  void (*accept_object)(void*);
  void (*begin_method)(const char* selector, int argc, const char**, void*);
  void (*accept_method)(const char* selector, int argc, const char**, void*);
  void (*begin_block)(int, const char**, void*);
  void (*accept_block)(int, const char**, void*);
};

void ft_init_parser(struct ft_parser* parser, struct ft_parser_actions* actions, struct ft_parser_input* input);
struct ft_parser_position ft_parser_get_position(struct ft_parser* p);

void ft_parser_parse_script(struct ft_parser* p, void* arg);
bool ft_parser_is_eof(struct ft_parser* p);

#endif
