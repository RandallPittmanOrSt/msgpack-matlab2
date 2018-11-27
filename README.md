# MessagePack for Matlab 

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

```matlab
    >> msgpack('<cmd> [<flag>[ <flag>[ ...]]]', ...)
```

...where `<cmd>` may be simply `setflags` or one of the other commands followed
by that command's arguments, and where each `<flag>` is one of the following,
prefixed with `+` to set or `-` to unset:

* `+unicode_strs` or `-unicode_strs`
  * **Set** - MessagePack strings are assumed to be UTF-8 and are unpacked to MATLAB's UTF-16, 
              and vis-versa
  * **Unset** - MessagePack strings are of unknown encoding and are unpacked as a uint8 array.
                When packing a `char` array, just try to pack MATLAB `mxChar`s if they are 
                smaller than `0x00ff` into the MessagePack string field.
* `+pack_u8_bin` or `-pack_u8_bin`
  * **Set** - MATLAB `uint8` arrays are packed into MessagePack bin messages.
  * **Unset** - MATLAB `uint8` arrays are packed into MessagePack arrays.
  * In both cases, non-`uint8` arrays are packed into MessagePack arrays.
    Use MATLAB's `typecast` to convert to `uint8` to pack other types as
    MessagePack bin messages.

## Issue

Since Matlab string is two-bytes (UTF16), following approach will not return correct string size

    >> msgpack('unpack', msgpack('pack', 'hello'))
    >> h e l l o

For correct string size, use
  
    >> msg = msgpack('pack', uint8('hello'))
    >> char(msgpack('unpack', msg))'
    >> hello
