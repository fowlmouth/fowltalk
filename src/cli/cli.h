#ifndef HEADER_CLI
#define HEADER_CLI

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct cli;
typedef void(*cli_cb)(struct cli*, const char***, const char**, void*);

struct cli_option
{
  const char* key;
  void* value;
  cli_cb cb;
};

struct cli
{
  int opt_count, opt_capacity;
  struct cli_option* options;
};

bool cli_init(struct cli* cli, int capacity);
void cli_deinit(struct cli* cli);

void cli_bool(struct cli* cli, const char* key, bool* value);
void cli_string(struct cli* cli, const char* key, const char** value);

void cli_parse(struct cli* cli, int argc, const char** argv);

void cli_print_help(struct cli* cli, const char* exe);


#endif
