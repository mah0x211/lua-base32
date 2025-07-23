# lua-base32

[![test](https://github.com/mah0x211/lua-base32/actions/workflows/test.yml/badge.svg)](https://github.com/mah0x211/lua-base32/actions/workflows/test.yml)
[![codecov](https://codecov.io/gh/mah0x211/lua-base32/branch/master/graph/badge.svg)](https://codecov.io/gh/mah0x211/lua-base32)

Base32 encoding/decoding module for Lua with support for both RFC 4648 and Crockford's Base32 formats.

## Installation

```bash
luarocks install base32
```

## Error Handling

the following functions return an `error` object created by https://github.com/mah0x211/lua-errno module.


## str = base32.encode(data [, format])

Encodes a string using Base32 encoding.

**Parameters:**

- `data:string`: The data to encode.
- `format:string`: The encoding format
    - `"rfc"`: RFC 4648 Base32 (default)
    - `"crockford"`: Crockford's Base32 encoding

**Returns:**

- `str:string`: The Base32 encoded string

**Example:**

```lua
local base32 = require("base32")

-- RFC 4648 Base32 (default)
print(base32.encode("foobar")) -- "MZXW6YTBOI======"

-- Crockford's Base32
print(base32.encode("foobar", "crockford")) -- "CSQPYRK1E8"
```

## str, err = base32.decode(data [, format])

Decodes a Base32 encoded string.

**Parameters:**

- `data:string`: The Base32 encoded string to decode
- `format:string`: The decoding format
    - `"rfc"`: RFC 4648 Base32 (default)
    - `"crockford"`: Crockford's Base32 encoding

**Returns:**

- `str:string`: The decoded data on success, or `nil` on failure
- `err:any`: nil and an error object on failure

**Example:**

```lua
local base32 = require("base32")

-- RFC 4648 Base32 (default)
print(base32.decode("MZXW6YTBOI======")) -- "foobar"

-- Crockford's Base32
print(base32.decode("CSQPYRK1E8", "crockford")) -- "foobar"

-- Error handling
local res, err = base32.decode("INVALID!")
if not res then
    print("Decode error:", err)
    -- Decode error:	./test.lua:10: in main chunk: [EILSEQ:92][base32.decode] Illegal byte sequence (Illegal character in Base32 string: '!' (0x21) at position 8)
end

```

## Encoding Formats

### RFC 4648 Base32

The standard Base32 encoding as defined in [RFC 4648](https://datatracker.ietf.org/doc/html/rfc4648#section-6).

**Characteristics:**

- Alphabet: `A-Z`, `2-7`
- Padding: Uses `=` for padding to make output length a multiple of 8
- Case-insensitive decoding
- Valid padding lengths: 0, 1, 3, 4, or 6 characters

**Example:**

```lua
local base32 = require("base32")

-- Encoding
base32.encode("")        -- ""
base32.encode("f")       -- "MY======"
base32.encode("fo")      -- "MZXQ===="
base32.encode("foo")     -- "MZXW6==="
base32.encode("foob")    -- "MZXW6YQ="
base32.encode("fooba")   -- "MZXW6YTB"
base32.encode("foobar")  -- "MZXW6YTBOI======"

-- Decoding (case-insensitive)
base32.decode("MZXW6YTBOI======")  -- "foobar"
base32.decode("mzxw6ytboi======")  -- "foobar"
```


### Crockford's Base32

An alternative Base32 encoding designed to be more human-friendly, as described at [crockford.com/base32.html](https://www.crockford.com/base32.html).

**Characteristics:**

- Alphabet: `0-9`, `A-H`, `J-K`, `M-N`, `P-T`, `V-Z` (excludes `I`, `L`, `O`, `U`)
- No padding characters
- Case-insensitive decoding
- Special character mappings:
  - `I` and `L` decode as `1`
  - `O` decodes as `0`
  - Hyphens (`-`) are ignored for readability
  - `U` is invalid

**Example:**

```lua
local base32 = require("base32")

-- Encoding
print(base32.encode("foobar", "crockford")) -- "CSQPYRK1E8"

-- Decoding with special mappings
print(base32.decode("CSQPYRK1E8", "crockford")) -- "foobar"
print(base32.decode("CSQPYRKIE8", "crockford")) -- "foobar" (I→1)
print(base32.decode("CSQPYRKLE8", "crockford")) -- "foobar" (L→1)
print(base32.decode("CS-QP-YRK1E8", "crockford")) -- "foobar" (hyphens ignored)
```

## Error Handling

The decode function returns `nil` and an error object when decoding fails:

```lua
local base32 = require("base32")

-- Invalid character
local result, err = base32.decode("MZXW6YT8") -- '8' is invalid in RFC 4648
if not result then
    print(err)
    -- ./test.lua:4: in main chunk: [EILSEQ:92][base32.decode] Illegal byte sequence (Illegal character in Base32 string: '8' (0x38) at position 8)
end

-- Invalid padding
result, err = base32.decode("MZXW6Y==") -- 2 padding chars is invalid
if not result then
    print(err)
    -- ./test.lua:10: in main chunk: [EINVAL:22][base32.decode] Invalid argument (RFC 4648 Base32 padding length must be 0, 1, 3, 4, or 6)
end

-- Invalid length for RFC format
result, err = base32.decode("MZXW6") -- Must be multiple of 8
if not result then
    print(err)
    -- ./test.lua:16: in main chunk: [EINVAL:22][base32.decode] Invalid argument (RFC 4648 Base32 requires input length to be a multiple of 8)
end
```

## License

MIT License

