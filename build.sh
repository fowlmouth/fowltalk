#!/bin/sh

set -e

THISDIR="$(cd $(dirname $0); pwd -P)"

BINDIR="$THISDIR/bin"
SRCDIR="$THISDIR/src/vm"
CLIDIR="$THISDIR/src/cli"
INCLUDEDIR="$THISDIR/include"

LINENOISE="$THISDIR/linenoise"

BINNAME="$BINDIR/fowltalk"
SONAME="$BINDIR/fowltalk.dylib"

DEBUG_CFLAGS="-O0 -g -DFT_DEBUG_MEMORY=1 -DFT_DEBUG_LEXER=0 -DFT_DEBUG_PARSER=1 -DFT_DEBUG_COMPILER=1 -DFT_DEBUG_BYTECODE_BUILDER=1 -DFT_DEBUG_VM=1 -DFT_DEBUG_VTABLE=1 -DFT_DEBUG_FOWLTALK=1 "
DEBUG2_CFLAGS="-fsanitize=address"

RELEASE_CFLAGS="-O1 -Wno-switch"

CFLAGS="-std=c17 -I$INCLUDEDIR"
DO_RUN=

while [ "$#" != 0 ]; do

  case "$1" in
    -release)
      CFLAGS="$CFLAGS $RELEASE_CFLAGS"
      BINNAME="$BINDIR/fowltalk"
      SONAME="$BINDIR/fowltalk.dylib"
      ;;

    -debug)
      CFLAGS="$CFLAGS $DEBUG_CFLAGS $DEBUG2_CFLAGS"
      BINNAME="$BINDIR/fowltalk-debug"
      SONAME="$BINDIR/fowltalk-debug.dylib"
      ;;

    -run)
      DO_RUN=1
      ;;
  esac

  shift

done

if [ ! -d "linenoise" ]; then
  git clone https://github.com/antirez/linenoise.git
fi


CSOURCES="fowltalk.c gc.c gc-ft.c gc-memory.c vm.c bytes.c environment.c vtable.c string.c array.c primitives.c compiler.c lexer.c parser-input.c parser.c bytecode-builder.c parser-validator.c"

FULL_CSOURCES=""
for source in $CSOURCES; do
  FULL_CSOURCES+="$SRCDIR/$source "
done

if [ ! -d "$BINDIR" ]; then
  mkdir "$BINDIR"
fi

cmd(){
  echo "$@"
  $@
}

cmd clang $CFLAGS $FULL_CSOURCES -o "$SONAME"  -dynamiclib
cmd clang $CFLAGS -o "$BINNAME" "$CLIDIR/fowltalk.c" "$CLIDIR/cli.c" "$LINENOISE/linenoise.c" -I"$LINENOISE" "$SONAME"
[ ! -z "$DO_RUN" ] && cmd "$BINNAME"
