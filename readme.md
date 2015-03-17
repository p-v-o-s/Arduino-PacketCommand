PacketCommand
=============
A simple light-weight Wiring/Arduino library to dispatch binary packet commands 
to user configurable handler functions.  The style of the API was inspired heavily 
by a fork of [Arduino-SerialCommand](https://github.com/p-v-o-s/Arduino-SerialCommand); 
the orginal version of that library was written by [Steven Cogswell](http://husks.wordpress.com) (published May 23, 2011 in his blog post ["A Minimal Arduino Library for Processing Serial Commands"](http://husks.wordpress.com/2011/05/23/a-minimal-arduino-library-for-processing-serial-commands/)).


The term "packet" here is used to refer to a variable length grouping of bytes that has a well-defined structure.  A 
valid packet must start with a variable length byte sequence that is 
interpreted as its "type ID".  A valid type ID is defined like this:
[```0x01-0xFE```] or [```0xFF01-0xFFFE```] or ```...```; that is, if it starts 
with a prefix [```0xFF```]*, then we include the next byte as part of the type ID, 
so that ```0x01``` is distinct from ```0xFF01``` and so on; this way, we get 
cheap extensibility.  Type IDs must not terminate with ```0xFF``` and must not 
include any null bytes (```0x00```), since those are reserved by the implementation.
The implementation uses a constant ```MAX_TYPE_ID_LEN``` to limit the worst-case
erroneous packet detection time.

Each instance of the ```PacketCommand``` class is an independent parser and dispatcher
for binary packets as defined above.  For each command, the client code should 
associate a type ID and name with a handler function in the global scope during the 
```setup()```  phase, using the ```addCommand``` method on the ```PacketCommand``` 
instance.  The handler functions must return ```void``` and take a single argument of 
type ```PacketCommand```, which is used to pass the currently dispatching instance to 
the handler - thus allowing for the use of multiple instances that share common handlers.  

Each complete binary packet can be fed to the ```PacketCommand``` object using
the ```readBuffer``` method.  If a registered type ID can be matched, then the current 
command handler for it is set on the object instance; otherwise, a default will
be set as the current command handler function if one has been been setup using 
```setDefaultHandler```.  The ```PacketCommand::CommandInfo``` structure of the
currently selected command can be retrieved using the ```getCurrentCommand``` method.
Finally, when the ```dispatch``` method is called, control is passed to the handler 
function, which can take advantage of methods on the ```PacketCommand``` instance for 
parsing common datatypes from the remainder of the packet, and can then trigger any 
side-effect actions to be taken.
