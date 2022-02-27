#!/bin/sh


cmd(){
  echo $@
  $@
}

cmd clang cli-test.c cli.c -o cli -g -O0 -std=c17
