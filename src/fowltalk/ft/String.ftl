
{
  newSize: size [
    __String_newSize: size
  ].

  size [
    __String_size
  ].

  at: index [
    __String_at: index
  ].

  at: index put: value [
    __String_at: index put: value
  ].

  from: lowIndex to: highIndex [
    self __String_copyFrom: lowIndex to: highIndex
  ].

  hashValue [
    __String_hashValue
  ].

} newSize: 0
