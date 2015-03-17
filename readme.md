PacketCommand
=============
A simple light-weight Wiring/Arduino library to dispatch binary packet commands 
to user configurable handler functions.  The term "packet" here is used to refer
to a variable length grouping of bytes that has a well-defined structure.  A 
valid packet must start with a variable length byte sequence that is 
interpretted as its "type" ID.  A valid type ID is defined like this:
```0x01-0xFE or 0xFF01-0xFFFE or ...```, that is, if it starts with ```0xFF```, 
then we include the next byte as part of the type, so that 0x01 is distinct from
0xFF01 and so on; this way we get cheap extensibility.  Type IDs should not
include any null bytes (```0x00```), since those are reserved by the implementation.

Each instance of the ```PacketCommand``` class is an independent parser and dispatcher
for binary packets as defined above.  For each command, the client code should 
associate a type ID and name with a handler function in the global scope during  ```setup()``` 
phase, using the ```addCommand``` method on the ```PacketCommand``` instance.
Each complete binary packet can be fed to the ```PacketCommand``` object using the 
```readBuffer``` method.  If a registered type ID can be matched, the current command
handler for it is set on the object instance; otherwise

and the ```PacketCommand::CommandInfo``` structure can be retrieved using the ```getCurrentCommand``` method;   which can take advantage of methods for parsing packet parsing and side-effect actions to be taken.
