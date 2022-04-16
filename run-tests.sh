#!/bin/bash

THISDIR="$(cd $(dirname $0); pwd -P)"

BINDIR="$THISDIR/bin"
BINNAME="$BINDIR/fowltalk"

TESTDIR="$THISDIR/tests/fowltalk"

TESTARGS="-minimal"

successful=0
tested=0

cd "$TESTDIR"
for input_filename in *.input; do
  output_filename="$(basename "$input_filename" .input).output"
  if [ -f "$output_filename" ]; then
    expected_output="$(cat "$output_filename")"
    actual_output="$(LD_LIBRARY_PATH="$BINDIR" $BINNAME $TESTARGS -f "$input_filename" 2>&1)"
    if [ "$expected_output" != "$actual_output" ]; then
      echo "failed test: $input_filename"
      echo "  expected: '$expected_output' got: '$actual_output'"
      echo
    else
      successful=$((successful+1))
    fi
    tested=$((tested+1))
  fi
done

echo "$successful / $tested successful tests"
