PacketCommand
=============
A simple light-weight Wiring/Arduino library to dispatch binary packet commands 
to user configurable handler functions.  The term "packet" here is used to refer
to a variable length grouping of bytes that has a well-defined structure.  A 
valid packet must start with a variable length byte sequence that is 
interpretted as its "type" ID.  A valid type ID is defined like this:
```0x01-0xFE or 0xFF01-0xFFFE or ...```, that is, if it starts with 0xFF, then 
we include the next byte as part of the type, so that 0x01 is distinct from 
0xFF01 and so on; this way we get cheap extensibility.  Type IDs that end
with 0x00 are reserved by the implementation and should not be used. The client 
code will associate a type ID with a handler function that takes care of the 
rest of the packet parsing and side-effect actions to be taken.
