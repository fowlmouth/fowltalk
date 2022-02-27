
{
  typeName = 'Nil'.

  isNil [
    true
  ].

  ifNil: nilBlock else: elseBlock [
    nilBlock value
  ].

  ifNil: nilBlock [
    nilBlock value
  ].

  ifTrue: trueBlock else: elseBlock [
    elseBlock value
  ].

  ifTrue: trueBlock [
    self
  ].

  ifFalse: falseBlock [
    self
  ].


}