# MessagePack for Matlab 

MEX bindings for msgpack-c (https://github.com/msgpack/msgpack-c)

Requires:
* msgpack-c development library > 1.1

install: 

```bash
mex -O msgpack.cc -lmsgpack
```

## API

### Packer:

```matlab
>> msg = msgpack('pack', var1, var2, ...)
```

### Unpacker:

```matlab
>> obj = msgpack('unpack', msg) 
```

return numericArray or charArray or LogicalArray if data are numeric otherwise return Cell or Struct
  
### Streaming unpacker:

```matlab
>> objs = msgpack('unpacker', msg)
```

return Cell containing numericArray, charArray, Cell or Struct

### Flags

Flags may be set that affect this and future calls of `msgpack()` as follows:

```matlab
>> msgpack('<cmd> [<flag>[ <flag>[ ...]]]', ...)
```

...where `<cmd>` may be simply `setflags` or one of the other commands followed
by that command's arguments, and where each `<flag>` is one of the following,
prefixed with `+` to set or `-` to unset:

* `+unicode_strs` or `-unicode_strs` (default is **set**)
  * **Set** - MessagePack strings are assumed to be UTF-8 and are unpacked to MATLAB's UTF-16, 
              and vis-versa
  * **Unset** - MessagePack strings are of unknown encoding and are unpacked as a uint8 array.
                When packing a `char` array, just try to pack MATLAB `mxChar`s if they are 
                smaller than `0x00ff` into the MessagePack string field.
* `+pack_u8_bin` or `-pack_u8_bin` (default is **unset**)
  * **Set** - MATLAB `uint8` arrays are packed into MessagePack bin messages.
  * **Unset** - MATLAB `uint8` arrays are packed into MessagePack arrays.
  * In both cases, non-`uint8` arrays are packed into MessagePack arrays.
    Use MATLAB's `typecast` to convert to `uint8` to pack other types as
    MessagePack bin messages.
* `+unpack_narrow` or `-unpack_narrow` (default is **unset**)
  * **Set** - When unpacking scalar numeric types, unpack to the narrowest type possible.
    * Positive integers -> smallest possible uint
    * Negative integers -> smallest possible int
    * Float32 -> single
    * Float64 -> double
  * **Unset** - Unpack all scalar numeric types to MATLAB double type.
  * In both cases, arrays of all the same numeric type are converted as follows:
    * Positive integers -> uint64
    * Negative integers -> int64
    * Float32 -> single
    * Float64 -> double
* `+unpack_map_as_cells` or `-unpack_map_as_cells` (default is **unset**)
  * **Set** - When unpacking a map, always unpack as a 2xN cell matrix of keys and values.
  * **Unset** - If all keys are strings, unpack a map to a struct. If not, generate a
                warning and unpack to a cell matrix as above.
