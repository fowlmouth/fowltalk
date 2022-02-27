#include "cli.h"
#include <stdio.h>

int main(int argc, const char** argv)
{
  bool run_tests = false;
  const char* file = NULL;

  struct cli cli;
  cli_init(&cli, 1);

  cli_bool(&cli, "-test", &run_tests);
  cli_string(&cli, "-file", &file);

  cli_parse(&cli, argc, argv);

  cli_deinit(&cli);

  printf("run tests: %d\nfile: '%s'\n", (int)run_tests, file);
  return 0;
}

