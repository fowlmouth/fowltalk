
{
  typeName = 'False'.

  ifTrue: trueBlock else: elseBlock [
    elseBlock value
  ].

  ifTrue: trueBlock [
    self
  ].

  ifFalse: falseBlock [
    falseBlock value
  ]

}