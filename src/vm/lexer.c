#ifndef FT_DEBUG_LEXER
#define FT_DEBUG_LEXER 0
#endif

#define FT_DEBUG FT_DEBUG_LEXER

#include "fowltalk/lexer.h"
#include "fowltalk/debug.h"

#include <stdbool.h>
#include <string.h>
#include <ctype.h>





 


int ft_parser_lexer_next_character(struct ft_parser_lexer* lex);

void ft_init_parser_lexer(struct ft_parser_lexer* lex, struct ft_parser_input* input)
{
  ft_debug("%ld %ld", lex->line, lex->col);
  memset(lex, 0, sizeof(struct ft_parser_lexer));
  lex->input = input;
  lex->current_character = 0;
  lex->line = 1;
  lex->col = -1;
  ft_parser_lexer_next_character(lex);
  ft_debug("parser first char: %d", lex->current_character);
  ft_debug("%ld %ld", lex->line, lex->col);
}

int ft_parser_lexer_next_character(struct ft_parser_lexer* lex)
{
  int c = ft_parser_input_vtable(lex->input)->next_char(lex->input);
  if(lex->current_character == '\n')
  {
    ++lex->line;
    lex->col = 0;
  }
  else
  {
    ++lex->col;
  }
  return lex->current_character = c;
}


int ft_parser_lexer_skip_whitespace(struct ft_parser_lexer* lex)
{
  int whitespace_count = 0;
  while(lex->current_character == ' ')
  {
    ft_parser_lexer_next_character(lex);
    ++whitespace_count;
  }
  return whitespace_count;
}

struct ft_parser_position ft_parser_lexer_get_position(struct ft_parser_lexer* lex)
{
  struct ft_parser_position pp = {
    ft_parser_input_vtable(lex->input)->get_position(lex->input),
    lex->line,
    lex->col
  };
  return pp;
}

void ft_parser_lexer_set_position(struct ft_parser_lexer* lex, struct ft_parser_position position)
{
  ft_parser_input_vtable(lex->input)->set_position(lex->input, position.position-1);
  lex->line = position.line;
  lex->col = position.col-1;
  lex->current_character = 0;
  ft_parser_lexer_next_character(lex);
}

bool is_operator_char(int character)
{
  switch(character)
  {
  case '!': case '@': case ',': case '&': case '|':
  case '+': case '-': case '*': case '/': case '%':
    return true;
  default:
    return false;
  }
}

bool ft_parser_lexer_read_token(struct ft_parser_lexer* lex, struct ft_parser_token* token, int error_buffer_size, char* error_buffer)
{
  ft_parser_lexer_skip_whitespace(lex);

  struct ft_parser_position p = ft_parser_lexer_get_position(lex);
  char* str = token->data;
  *str = 0;
  token->begin = p;
  token->type = ftptt_eof;

  bool is_negative = false;

#define SIMPLETOK(ttype, tstr) \
  do{ \
    token->type = (ttype); \
    memcpy(token->data, (tstr), strlen(tstr)+1); \
    ft_parser_lexer_next_character(lex); \
  }while(0)

  int cc = lex->current_character;
  switch(cc)
  {
  case '\n':
    SIMPLETOK(ftptt_newline, "\n");
    break;

  case '(':
    SIMPLETOK(ftptt_open_paren, "(");
    break;
  case ')':
    SIMPLETOK(ftptt_close_paren, ")");
    break;

  case '{':
    SIMPLETOK(ftptt_open_brace, "{");
    break;
  case '}':
    SIMPLETOK(ftptt_close_brace, "}");
    break;

  case '[':
    SIMPLETOK(ftptt_open_bracket, "[");
    break;
  case ']':
    SIMPLETOK(ftptt_close_bracket, "]");
    break;

  case '<':
    SIMPLETOK(ftptt_open_chevron, "<");
    break;
  case '>':
    SIMPLETOK(ftptt_close_chevron, ">");
    break;

  case '.':
    SIMPLETOK(ftptt_period, ".");
    break;
  case '^':
    SIMPLETOK(ftptt_caret, "^");
    break;

  case '|':
    SIMPLETOK(ftptt_pipe, "|");
    break;

  case '\'':
    cc = ft_parser_lexer_next_character(lex);
    while(cc > 0 && cc != '\'')
    {
      switch(cc)
      {
      case '\\':
        cc = ft_parser_lexer_next_character(lex);
        switch(cc)
        {
        case 'n':
          *str++ = '\n';
          break;
        case 't':
          *str++ = '\t';
          break;
        default:
          snprintf(error_buffer, error_buffer_size, "invalid escape character '%c'", (char)cc);
          ft_debug("%s", error_buffer);
          return false;
        }
        break;

      default:
        *str++ = cc;
        break;
      }
      cc = ft_parser_lexer_next_character(lex);
    }
    if(cc != '\'')
    {
      snprintf(error_buffer, error_buffer_size, "unterminated string");
      return false;
    }
    ft_parser_lexer_next_character(lex);
    token->type = ftptt_string;
    *str++ = 0;
    return true;

  case ':':
    cc = ft_parser_lexer_next_character(lex);
    if(cc == '=')
    {
      SIMPLETOK(ftptt_assignment, ":=");
      return true;
    }
    snprintf(error_buffer, error_buffer_size, "invalid token ':%c'", (char)cc);
    return false;

  case '!': case '@': case ',': case '&':
  case '+': case '*': case '/': case '%':
    token->type = ftptt_operator;
    while(is_operator_char(cc) || cc == '=')
    {
      *str++ = cc;
      cc = ft_parser_lexer_next_character(lex);
    }
    *str++ = 0;
    return true;

  case '=':
    cc = ft_parser_lexer_next_character(lex);
    if(cc == '=')
    {
      SIMPLETOK(ftptt_operator, "==");
      return true;
    }
    token->type = ftptt_assignment;
    *str++ = '=';
    *str++ = 0;
    return true;

  case '-':
    cc = ft_parser_lexer_next_character(lex);
    *str++ = '-';
    if(!isdigit(cc))
    {
      token->type = ftptt_operator;
      while(is_operator_char(cc) || cc == '=')
      {
        *str++ = cc;
      }
      *str++ = 0;
      return true;
    }
    is_negative = true;
    // fall-through used here intentionally

  case '0': case '1': case '2':
  case '3': case '4': case '5':
  case '6': case '7': case '9':

    token->type = ftptt_integer;
    token->int_value = 0;
    while(isdigit(cc))
    {
      token->int_value = (token->int_value * 10) + (cc - '0');
      *str++ = (char)cc;
      cc = ft_parser_lexer_next_character(lex);
    }
    if(is_negative)
    {
      token->int_value *= -1;
    }
    *str++ = 0;

    return true;

  case FT_PARSER_EOF_VALUE:

    token->type = ftptt_eof;
    // return false;
    return true;


  default:

    if((cc >= 'a' && cc <= 'z')
        || (cc >= 'A' && cc <= 'Z')
        || cc == '_')
    {
      // identifier
      token->type = ftptt_identifier;

      while((cc >= 'a' && cc <= 'z')
        || (cc >= 'A' && cc <= 'Z')
        || cc == '_'
        || (cc >= '0' && cc <= '9'))
      {
        *str++ = cc;
        cc = ft_parser_lexer_next_character(lex);
      }

      if(cc == ':')
      {
        token->type = ftptt_keyword;
        *str++ = cc;
        ft_parser_lexer_next_character(lex);
      }
      *str++ = 0;

      return true;
    }

    ft_debug("unknown char %d", cc);
    return false;
  }

  return true;

#undef SIMPLETOK
}






void ft_parser_lexer_test(struct ft_parser_input* input)
{
  // int result = 0;
  // while(result != ftpi_eof_value)
  // {
  //   result = ft_parser_input_vtable(input)->next_char(input);
  //   printf("|%-4d| %c\n", result, (char)result);
  // }

  struct ft_parser_lexer lex;
  ft_init_parser_lexer(&lex, input);

  int starting_character = lex.current_character;
  struct ft_parser_position starting_position = ft_parser_lexer_get_position(&lex);

  ft_debug(" type pos  line col   token");

  char error_buffer[1024];

  struct ft_parser_token token;
  while(ft_parser_lexer_read_token(&lex, &token, 1024, error_buffer))
  {
    ft_debug(" %-4d %-4ld %-4ld %-4ld  %s", token.type, token.begin.position, token.begin.line, token.begin.col, token.data);
    if(token.type == ftptt_eof)
      break;
  }

  ft_parser_lexer_set_position(&lex, starting_position);
  if(lex.current_character != starting_character)
  {
    ft_debug("!! error char '%d' != char '%d'", (int)lex.current_character, (int)starting_character);
  }
  struct ft_parser_position second_starting_position = ft_parser_lexer_get_position(&lex);
  if(memcmp(&starting_position, &second_starting_position, sizeof(struct ft_parser_position)))
  {
    ft_debug("!! position mismatch (_,%ld,%ld) expected (%ld,%ld,%ld)",
      lex.line, lex.col,
      starting_position.position, starting_position.line, starting_position.col);
  }

}
