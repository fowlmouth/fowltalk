#ifndef H_FOWLTALK_LEXER
#define H_FOWLTALK_LEXER

#include "fowltalk.h"
#include "parser-input.h"




enum ft_parser_token_type
{
  ftptt_eof,
  ftptt_integer, ftptt_string,
  ftptt_newline, ftptt_pipe,
  ftptt_assignment, ftptt_operator,
  ftptt_keyword, ftptt_identifier,
  ftptt_open_paren, ftptt_close_paren,
  ftptt_open_brace, ftptt_close_brace,
  ftptt_open_bracket, ftptt_close_bracket,
  ftptt_open_chevron, ftptt_close_chevron,
  ftptt_period, ftptt_minus, ftptt_caret,

  ftptt__count
};

struct ft_parser_position
{
  ft_parser_input_position_t position;
  long line, col;
};

#define FT_PARSER_TOKEN_SIZE 256

struct ft_parser_token
{
  enum ft_parser_token_type type;
  struct ft_parser_position begin, end;
  ft_integer_t int_value;
  char data[FT_PARSER_TOKEN_SIZE];
};

struct ft_parser_lexer
{
  struct ft_parser_input* input;
  long line, col;
  int current_character;
  // struct ft_parser_token next;
};


void ft_init_parser_lexer(struct ft_parser_lexer* lex, struct ft_parser_input* input);
// bool ft_parser_lexer_read_token(struct ft_parser_lexer* lex, struct ft_parser_token* token);
bool ft_parser_lexer_read_token(struct ft_parser_lexer* lex, struct ft_parser_token* token, int buffer_size, char* buffer);

void ft_parser_lexer_test(struct ft_parser_input* input);
struct ft_parser_position ft_parser_lexer_get_position(struct ft_parser_lexer* lex);
void ft_parser_lexer_set_position(struct ft_parser_lexer* lex, struct ft_parser_position position);

#endif
