nil = uint8(hex2dec('c0'));
fixmap = hex2dec('80'); % (up to 0x8f, e.g. fixmap+11)
%% array of 2.5, 3.5, 2.4e8
packed = uint8([147 ...
                203 ...
                064 004 000 000 000 000 000 000 ...
                203 ...
                064 012 000 000 000 000 000 000 ...
                203 ...
                065 172 156 056 000 000 000 000 ]);
unpacked = msgpack('unpack', packed);
expected = [2.5, 3.5, 2.4e8];
assert(numel(expected) == numel(unpacked));
assert(strcmp(class(expected), class(unpacked)));
for i = 1:numel(expected)
    assert(strcmp(class(expected(i)), class(unpacked(i))), 'Wrong class for %d', i);
    assert(expected(i) == unpacked(i), 'Wrong value for %d', i);
end

%% array  [2.5, false, 2.4e8]
packed = uint8([147 ...
                203 ...
                064 004 000 000 000 000 000 000 ...
                194 ...
                203 ...
                065 172 156 056 000 000 000 000 ]);
unpacked = msgpack('unpack', packed);
expected = {2.5, false, 2.4e8};
assert(numel(expected) == numel(unpacked));
assert(strcmp(class(expected), class(unpacked)));
for i = 1:numel(expected)
    assert(strcmp(class(expected{i}), class(unpacked{i})), 'Wrong class for %d', i);
    assert(expected{i} == unpacked{i}, 'Wrong value for %d', i);
end

%% array [2.5, nil, 2.4e8]
msgpack('reset_flags');
msgpack('set_flags +unpack_nil_NaN -unpack_nil_array_skip')
packed = uint8([147 ...
                203 ...
                064 004 000 000 000 000 000 000 ...
                nil ...
                203 ...
                065 172 156 056 000 000 000 000 ]);
unpacked = msgpack('unpack', packed);
expected = [2.5, NaN, 2.4e8];
assert(numel(expected) == numel(unpacked), 'Different number of elements');
assert(strcmp(class(expected), class(unpacked)), 'Different class');
for i = 1:numel(expected)
    assert(strcmp(class(expected(i)), class(unpacked(i))), 'Wrong class for %d', i);
    if isnan(expected(i))
        assert(isnan(unpacked(i)), 'Should be NaN');
    else
        assert(expected(i) == unpacked(i), 'Wrong value for %d', i);
    end
end

%% skip nil
msgpack('set_flags +unpack_nil_array_skip');
unpacked = msgpack('unpack', packed);
expected = [2.5, 2.4e8];
assert(numel(expected) == numel(unpacked));
assert(strcmp(class(expected), class(unpacked)));
for i = 1:numel(expected)
    assert(strcmp(class(expected(i)), class(unpacked(i))), 'Wrong class for %d', i);
    assert(expected(i) == unpacked(i), 'Wrong value for %d', i);
end

%% all nil
msgpack('set_flags -unpack_nil_array_skip')
packed = [147, nil, nil, nil];
unpacked = msgpack('unpack', packed);
for i = 1:numel(unpacked)
    assert(isnan(unpacked(i)), "Should be NaN");
end

%% map with nil
% map with 2 elements, x: nil, y: {}
packed = uint8([fixmap+2, 161, uint8('x'), nil, 161, uint8('y'), hex2dec('90')]);
unpacked = msgpack('unpack +unpack_nil_NaN', packed);
assert(isstruct(unpacked));
assert(isnan(unpacked.x));

%% map with non-str keys unpacked to cell array
msgpack('set_flags +unpack_narrow');
warning('off', 'msgpack:non_str_map_keys');
% map with 2 elements, uint(3): double(3), y: int(-3)
packed = uint8([fixmap+2, ...
                3, 203, 64, 8, 0, 0, 0, 0, 0, 0, ...
                161, uint8('y'), 253]);
unpacked = msgpack('unpack', packed);
assert(iscell(unpacked), 'Should be cell');
assert(all(size(unpacked) == [2, 2]), 'Wrong size');
expected = {uint8(3), 3.0; 'y', int8(-3)};

%% map as cell array
msgpack('set_flags +unpack_map_as_cells');
%map with 3 elements, all str keys: 'ab': uint(5), 'cd': double(32.8), 'def': int(-3)
packed = uint8([fixmap+3, ...
                162, uint8('ab'), 5, ...
                162, uint8('cd'), 203, 64, 64, 102, 102, 102, 102, 102, 102, ...
                163, uint8('def'), 253]);
unpacked = msgpack('unpack', packed);
assert(iscell(unpacked));
assert(all(size(unpacked) == [2,3]), 'Wrong cell array size.');
expected = {'ab', 'cd', 'def'; uint8(5), 32.8, int8(-3)};
for i=1:numel(expected)
    assert(strcmp(class(expected{i}), class(unpacked{i})), 'Wrong class for elementd %d', i);
    if ischar(expected{i})
        assert(strcmp(expected{i}, unpacked{i}), 'Mismatch for element %d', i);
    else
        assert(expected{i} == unpacked{i}, 'Mismatch for element %d', i);
    end
end

%% all passed
disp('All tests passed.');
