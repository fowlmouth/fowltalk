
Fowltalk is a little prototype-based programming language. 

### Interpreter Status

* ✅ record stack effect of opcodes during building
* ✅ implement non-local return for blocks
* 📝 implement image saving/loading
* ✅ implement test runner
* ✅ unhandled messages send 'unknownMessage:withArguments:'
* 📝 add a logging library, more than just ft_debug()
* 📝 implement a garbage collector


### Virtual Machine

The VM is a simple stack machine with 8 instructions

* _literal index_ - load literal onto the stack
* _send argc_ - pop a selector and `argc` arguments from the stack and invoke the selector on the arguments (receiver is argument #0)
* _self-send argc_ - pop a selector and `argc` arguments from the stack and invoke the selector on the current activation context
* _return contexts_ - return through `context` activation contexts
* _init-slot index_ - initialize a slot
* _pop count_ - pop `count` from the stack
* _push-context_ - push a special object onto the stack
* _extend arg_ - extend the argument to the next opcode

#### Primitives

* `__VTable_instantiate` - 
* `__Object_clone` - return a shallow clone of the receiver
* `__Object_respondsTo:` - return a boolean for whether or 
* `__Object_copyAddingSlot:value:` 
* `__Object_copyRemovingSlot:` 
* `__Object_updateSlot:value:` - 
* `__Object_fetchSlot:` - fetch a slot value without evaluating it
* `__Object_addStaticSlot:value:` - copy the vtable and add a static slot
* `__Object_vtable` - fetch the vtable for an object

* `__String_concat:` - join two strings in a new copy
* `__String_print` - print out the receiver to stdout
* `__String_printLn` - print out the string and a newline to stdout
* `__String_readFile` - treat the receiver as a filename and read the contents
* `__String_compileAnonymousMethod` - treating the receiver as Fowltalk source code, compile into an anonymous method

* `__Integer_add:`, `__Integer_subtract:`, `__Integer_multiply:`, `__Integer_divide:` - math operations for integers
* `__Integer_print`, `__Integer_printLn` - printing functions for integers 

* `__VM_activateMethodContext` - switch to the receiver as a method context in the current vm


