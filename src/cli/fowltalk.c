#include "fowltalk.h"
#include "fowltalk/environment.h"
#include "fowltalk/primitives.h"
#include "fowltalk/compiler.h"
#include "fowltalk/vm.h"
#include "fowltalk/parser-validator.h"
#include <stdio.h>


#include "cli.h"

#include "linenoise.h"


void print_error(struct ft_virtual_machine* vm)
{
  char error_string_buffer[256];
  ft_vm_describe_error(vm, error_string_buffer, 256);
  fprintf(stderr, "execution error: %s", error_string_buffer);
}

int main(int argc, const char** argv)
{
  bool run_tests = false,
    minimal_env = false,
    help = false,
    repl = false;
  const char* run_file_path = NULL,
    *platform_path = "src/fowltalk/ft",
    *platform_name = "AutoLoadingModule",
    *eval_string = NULL;

  {
    struct cli cli;
    cli_init(&cli, 8);

    cli_bool(&cli, "-test", &run_tests);
    cli_string(&cli, "-file", &run_file_path);
    cli_string(&cli, "-f", &run_file_path);
    cli_string(&cli, "-e", &eval_string);

    cli_string(&cli, "-src", &platform_path);
    cli_string(&cli, "-platform", &platform_name);
    cli_bool(&cli, "-minimal-platform", &minimal_env);
    cli_bool(&cli, "-minimal", &minimal_env);

    cli_bool(&cli, "-help", &help);
    cli_bool(&cli, "-repl", &repl);

    cli_parse(&cli, argc, argv);

    if(help)
    {
      cli_print_help(&cli, argv[0]);
      return 0;
    }

    cli_deinit(&cli);
  }

  struct ft_environment* env = minimal_env
    ? ft_new_minimal_environment()
    : ft_new_environment(platform_path, platform_name);

  struct ft_allocator* allocator = ft_environment_allocator(env);

  struct ft_compiler_options compiler_options;
  ft_init_compiler_options(env, &compiler_options);

  if(run_file_path)
  {

    FILE* file = fopen(run_file_path, "r");
    oop method = ft_compile_file(env, &compiler_options, file);
    fclose(file);

    if(method)
    {
      struct ft_vm_quick_send_options opt = {
        env,
        ft_simple_lookup(ft_environment_lobby(env), ft_environment_intern(env, "VirtualMachineVT")),
        1024
      };
      oop result = ft_vm_quick_activate_and_run(&opt, method, OOP_NIL, print_error);
    }
    else
    {
      fprintf(stderr, "failed to compile file\n");
      return 1;
    }
  }

  if(eval_string)
  {
    oop method = ft_compile_string(env, &compiler_options, strlen(eval_string), eval_string);

    if(method)
    {
      struct ft_vm_quick_send_options opt = {
        env,
        ft_simple_lookup(ft_environment_lobby(env), ft_environment_intern(env, "VirtualMachineVT")),
        1024
      };
      oop result = ft_vm_quick_activate_and_run(&opt, method, OOP_NIL, print_error);
    }
    else
    {
      fprintf(stderr, "failed to compile eval string\n");
      return 1;
    }
  }

  if(repl)
  {
    // linenoiseSetMultiLine(1);
    char* line;
    char* buffer_begin = calloc(1, 1024);
    int buffer_capacity = 1024;
    char* buffer = buffer_begin;

    while((line = linenoise("> ")) != NULL)
    {
      printf("You wrote '%s'\n", line);
      int len = strlen(line);
      if(buffer + len > buffer_begin + buffer_capacity)
      {
        // TODO resize buffer
        fprintf(stderr, "repl buffer overflow\n");
        return 1;
      }
      memcpy(buffer, line, len);
      buffer[len] = 0;
      buffer += len;

      linenoiseFree(line);

      struct ft_parser_string_input input;
      ft_init_parser_string_input(&input, buffer-buffer_begin, buffer_begin);

      if(ft_validate_source((struct ft_parser_input*)&input))
      {
        printf("valid input!\n");
      }
      else
      {
        printf("invalid input\n");
      }
      
    }
  }

  return 0;
}
