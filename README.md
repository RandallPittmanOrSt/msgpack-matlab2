# MessagePack for Matlab 
=============
MEX bindings for msgpack-c (https://github.com/msgpack/msgpack-c)

Requires:
* msgpack-c development library > 1.1

install: 
  
    mex -O msgpack.cc -lmsgpack

## API

### Packer:

    >> msg = msgpack('pack', var1, var2, ...)

### Unpacker:

    >> obj = msgpack('unpack', msg) 
    
return numericArray or charArray or LogicalArray if data are numeric otherwise return Cell or Struct
  
### Streaming unpacker:

    >> objs = msgpack('unpacker', msg)
  
return Cell containing numericArray, charArray, Cell or Struct

### Flags

Flags may be set that affect this and future calls of `msgpack()` as follows:

    >> msgpack(cmd, [±flag[, ±flag[, ...]]])

...where `cmd` may be simply `'setflags'` or may be one of the other commands, and where the `flag`s
are either set (`'+flag'` or unset (`'-flag'`)) for the following flags:

* `unicode_strs`
  * **Set** - MessagePack strings are assumed to be UTF-8 and are unpacked to MATLAB's UTF-16, and vis-versa
  * **Unset** - MessagePack strings are of unknown encoding and are unpacked as a uint8 array.
                When packing a `char` array, just try to pack MATLAB `mxChar`s if they are 
                smaller than `0x00ff` into the MessagePack string field.

## Issue

Since Matlab string is two-bytes (UTF16), following approach will not return correct string size

    >> msgpack('unpack', msgpack('pack', 'hello'))
    >> h e l l o

For correct string size, use
  
    >> msg = msgpack('pack', uint8('hello'))
    >> char(msgpack('unpack', msg))'
    >> hello
