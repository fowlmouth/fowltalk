
x = 4.

block0 = [
  x __Integer_multiply: 2
].

block0 value __Integer_printLn.

block1 = [|n|
  x __Integer_multiply: n
].

(block1 value: 3) __Integer_printLn.
