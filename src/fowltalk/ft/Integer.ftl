{
  add: rhs [
    __Integer_add: rhs
  ].
  subtract: rhs [
    __Integer_subtract: rhs
  ].
  multiply: rhs [
    __Integer_multiply: rhs
  ].
  divide: rhs [
    __Integer_divide: rhs
  ].

  bitAnd: rhs [
    __Integer_bitwiseAnd: rhs
  ].
  bitOr: rhs [
    __Integer_bitwiseAnd: rhs

  ].
  bitXor: rhs [
    __Integer_bitwiseAnd: rhs

  ].
  bitNot [
    __Integer_bitwiseNot
  ].
  shiftRight: rhs [
    __Integer_shiftRight: rhs
  ].
  shiftLeft: rhs [
    __Integer_shiftLeft: rhs
  ].

  + rhs [
    __Integer_add: rhs
  ].
  - rhs [
    __Integer_subtract: rhs
  ].
  * rhs [
    __Integer_multiply: rhs
  ].
  / rhs [
    __Integer_divide: rhs
  ].

  & rhs [
    __Integer_bitwiseAnd: rhs
  ].
  | rhs [
    __Integer_bitwiseAnd: rhs

  ].
  ^ rhs [
    __Integer_bitwiseAnd: rhs

  ].
  
  >> rhs [
    __Integer_shiftRight: rhs
  ].
  << rhs [
    __Integer_shiftLeft: rhs
  ].
}
