#include "cli.h"

bool cli_init(struct cli* cli, int capacity)
{
  if(capacity < 1)
  {
    capacity = 8;
  }
  void* opts = calloc(sizeof(struct cli_option), capacity);
  if(!opts)
  {
    return false;
  }

  cli->opt_count = 0;
  cli->opt_capacity = capacity;
  cli->options = (struct cli_option*)opts;
  return true;
}

void cli_deinit(struct cli* cli)
{
  if(cli->options)
  {
    free(cli->options);
  }
  memset(cli, 0, sizeof(struct cli));
}

struct cli_option* cli_next_opt(struct cli* cli)
{
  if(cli->opt_count == cli->opt_capacity)
  {
    int new_capacity = cli->opt_capacity * 2;
    if(!new_capacity) // cant happen if cli_init is used
    {
      new_capacity = 4;
    }
    struct cli_option* new_opts = calloc(sizeof(struct cli_option), new_capacity);
    memcpy(new_opts, cli->options, sizeof(struct cli_option) * cli->opt_capacity);
    if(cli->options)
    {
      free(cli->options);
    }
    cli->opt_capacity = new_capacity;
    cli->options = new_opts;
  }

  return cli->options + cli->opt_count++;
}




void cli_bool_cb(struct cli* cli, const char*** input, const char** end, void* value)
{
  ++*input;
  bool* bool_value = (bool*)value;
  *bool_value = true;
}

void cli_bool(struct cli* cli, const char* key, bool* value)
{
  struct cli_option* opt = cli_next_opt(cli);
  opt->key = key;
  opt->value = (void*)value;
  opt->cb = cli_bool_cb;
}





void cli_string_cb(struct cli* cli, const char*** input, const char** end, void* value)
{
  const char** str_value = (const char**)value;
  *str_value = *++*input;
  ++*input;
}

void cli_string(struct cli* cli, const char* key, const char** value)
{
  struct cli_option* opt = cli_next_opt(cli);
  opt->key = key;
  opt->value = (void*)value;
  opt->cb = cli_string_cb;
}




bool cli_match_opt(struct cli* cli, const char* arg, struct cli_option** out)
{
  for(int i = 0; i < cli->opt_count; ++i)
  {
    struct cli_option* opt = cli->options + i;

    if(!strcmp(arg, opt->key))
    {
      *out = opt;
      return true;
    }
  }
  return false;
}

bool cli_parse_arg(struct cli* cli, const char*** input, const char** input_end)
{
  const char* str = **input;
  int len = strlen(str);
  if(len && str[0] == '-')
  {
    struct cli_option* opt;
    if(cli_match_opt(cli, str, &opt))
    {
      if(opt->cb)
      {
        const char** old_input = *input;
        opt->cb(cli, input, input_end, opt->value);
        return *input != old_input;
      }
    }
  }
  return false;
}

void cli_parse(struct cli* cli, int argc, const char** argv)
{
  const char** argv_end = argv + argc;
  ++argv;

  while(argv != argv_end)
  {
    if(!cli_parse_arg(cli, &argv, argv_end))
    {
      break;
    }
  }

}


#include <stdio.h>

void cli_print_help(struct cli* cli, const char* exe)
{
  printf("usage: %s\n\n", exe);
  for(int i = 0; i < cli->opt_count; ++i)
  {
    struct cli_option* opt = cli->options + i;
    printf("  %s\n", opt->key);
  }
}


