
var1 = 42.

{
  typeName = 'True'.

  ifTrue: trueBlock else: elseBlock [
    trueBlock value
  ].

  ifTrue: trueBlock [
    trueBlock value
  ].

  ifFalse: falseBlock [
    self
  ]

}
