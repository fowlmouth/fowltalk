
'Lobby slots:' __String_printLn.
__Lobby __Object_printSlots.

newLobby = {
  globals* := {
    VTable = __Lobby VTable.
    VTableVT = __Lobby VTableVT.
    BytecodePrototype = __Lobby BytecodePrototype.
    String = __Lobby String.
    Bytes = __Lobby Bytes.
    Array = __Lobby Array.
  }.

  libraryPath := 0.

  loadingInProgress = {}.
  loadingFailed = {}.

  unknownMessage: selector withArguments: arguments [
    'UNKNOWN MESSAGE: ' __String_print.
    selector __String_printLn.

    globals: (globals __Object_copyAddingSlot: selector value: loadingInProgress).

    block = self loadModule: selector.

    obj := block value.
    (obj __Object_respondsTo: 'loadPlatform:')
      ifTrue: [
        obj: (obj loadPlatform: self)
      ].

    globals __Object_updateSlot: selector value: obj.

    obj

  ].

  modulePath: moduleName [
    ((libraryPath __String_concat: '/') __String_concat: moduleName) __String_concat: '.ftl'

  ].

  loadModule: moduleName [
    fullPath = modulePath: moduleName.
    fileContents = fullPath __String_readFile.

    {
      value = fileContents __String_compileAnonymousMethod
    }.
  ].
}.

bootstrapModule = [|moduleName|
  module = newLobby loadModule: moduleName.
  newGlobals = newLobby globals __Object_addStaticSlot: moduleName value: (module value).
  newLobby globals: newGlobals
].

newLobby libraryPath: 'src/fowltalk/ft'.

bootstrapModule value: 'True'.
bootstrapModule value: 'False'.
bootstrapModule value: 'Integer'.

'True: ' __String_print.
newLobby True __Object_printSlots.

'False: ' __String_print.
newLobby False __Object_printSlots.

'String: ' __String_print.
newLobby String __Object_printSlots.

newLobby.
