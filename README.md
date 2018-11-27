MessagePack for Matlab 
=============
MEX bindings for msgpack-c (https://github.com/msgpack/msgpack-c)

Requires:
* msgpack-c development library > 1.1

install: 
  
    mex -O msgpack.cc -lmsgpack

API

Packer:

    >> msg = msgpack('pack', var1, var2, ...)

Unpacker:

    >> obj = msgpack('unpack', msg) 
    
return numericArray or charArray or LogicalArray if data are numeric otherwise return Cell or Struct
  
Streaming unpacker:

    >> objs = msgpack('unpacker', msg)
  
return Cell containing numericArray, charArray, Cell or Struct


#Issue

Since Matlab string is two-bytes (UTF16), following approach will not return correct string size

    >> msgpack('unpack', msgpack('pack', 'hello'))
    >> h e l l o

For correct string size, use
  
    >> msg = msgpack('pack', uint8('hello'))
    >> char(msgpack('unpack', msg))'
    >> hello
