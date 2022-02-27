
{
    isNil [
      false
    ].

    ifNil: nilBlock else: elseBlock [
      elseBlock value
    ].

    ifNil: nilBlock [
      self
    ].

    vtable [
      __Object_vtable
    ].


  }
}