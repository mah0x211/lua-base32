#!/usr/bin/env lua

local base32 = require("base32")

-- Test framework
local tests = {}
local failed = 0
local passed = 0

local function test(name, fn)
    tests[#tests + 1] = {
        name = name,
        fn = fn,
    }
end

local function run_tests()
    print("Running base32 tests...")
    for _, t in ipairs(tests) do
        local ok, err = pcall(t.fn)
        if ok then
            passed = passed + 1
            print(string.format("✓ %s", t.name))
        else
            failed = failed + 1
            print(string.format("✗ %s: %s", t.name, err))
        end
    end
    print(string.format("\nPassed: %d, Failed: %d", passed, failed))
    os.exit(failed > 0 and 1 or 0)
end

-- Helper function to compare values
local function assert_eq(got, expected, msg)
    if got ~= expected then
        error(string.format("%s: expected %q, got %q",
                            msg or "assertion failed", expected, got))
    end
end

-- RFC 4648 Base32 Tests

test("test_rfc_test_vectors", function()
    -- Test all RFC 4648 Test Vectors
    local test_vectors = {
        {
            "",
            "",
        },
        {
            "f",
            "MY======",
        },
        {
            "fo",
            "MZXQ====",
        },
        {
            "foo",
            "MZXW6===",
        },
        {
            "foob",
            "MZXW6YQ=",
        },
        {
            "fooba",
            "MZXW6YTB",
        },
        {
            "foobar",
            "MZXW6YTBOI======",
        },
    }

    for _, vector in ipairs(test_vectors) do
        local input, expected = vector[1], vector[2]

        -- Test encoding
        local encoded = base32.encode(input)
        assert_eq(encoded, expected,
                  string.format("encode RFC vector: %q", input))

        -- Test decoding
        local decoded = base32.decode(expected)
        assert_eq(decoded, input,
                  string.format("decode RFC vector: %q", expected))
    end
end)

test("test_rfc_padding_validation", function()
    -- Test valid padding patterns (0, 1, 3, 4, 6)
    local valid_padding = {
        {
            "MZXW6YTB",
            "fooba",
            0,
        }, -- 0 padding
        {
            "MZXW6YQ=",
            "foob",
            1,
        }, -- 1 padding
        {
            "MZXW6===",
            "foo",
            3,
        }, -- 3 padding
        {
            "MZXQ====",
            "fo",
            4,
        }, -- 4 padding
        {
            "MY======",
            "f",
            6,
        }, -- 6 padding
    }

    for _, case in ipairs(valid_padding) do
        local encoded, expected, pad_count = case[1], case[2], case[3]
        local result = base32.decode(encoded)
        assert_eq(result, expected, string.format("valid %d padding", pad_count))
    end

    -- Test invalid padding patterns (2, 5, 7)
    local invalid_padding = {
        {
            "MZXW6Y==",
            2,
        }, -- 2 padding
        {
            "MZX=====",
            5,
        }, -- 5 padding
        {
            "M=======",
            7,
        }, -- 7 padding
    }

    for _, case in ipairs(invalid_padding) do
        local encoded, pad_count = case[1], case[2]
        local res, err = base32.decode(encoded)
        assert(not res, string.format("should reject %d padding", pad_count))
        assert(tostring(err):match("padding length must be 0, 1, 3, 4, or 6"),
               string.format(
                   "error should mention padding length must be 0, 1, 3, 4, or 6",
                   pad_count))
    end
end)

test("test_rfc_character_validation", function()
    -- Test case insensitive decoding
    local test_string = "MZXW6YTBOI======"
    local upper = base32.decode(test_string)
    local lower = base32.decode(test_string:lower())
    local mixed = base32.decode("MzXw6YtBoI======")

    assert_eq(upper, "foobar", "uppercase decode")
    assert_eq(lower, "foobar", "lowercase decode")
    assert_eq(mixed, "foobar", "mixed case decode")

    -- Test invalid characters for RFC 4648
    local invalid_chars = {
        "0",
        "1",
        "8",
        "9",
        "!",
    }
    for _, char in ipairs(invalid_chars) do
        local invalid_string = "MZXW6YT" .. char
        local res, err = base32.decode(invalid_string)
        assert(not res,
               string.format("should reject invalid character '%s'", char))
        assert(tostring(err):match("Illegal character in Base32 string"))
    end
end)

test("test_rfc_length_validation", function()
    -- RFC requires input length to be multiple of 8
    local invalid_lengths = {
        "MZXW6YT",
        "MZXW6",
        "M",
    }
    for _, invalid in ipairs(invalid_lengths) do
        local res, err = base32.decode(invalid)
        assert(not res,
               string.format("should reject length %d", string.len(invalid)))
        assert(tostring(err):match("input length to be a multiple of 8"),
               "error should mention invalid length")
    end
end)

test("test_rfc_binary_data", function()
    -- Test binary data handling
    local binary = string.char(0, 1, 2, 3, 4, 5, 6, 7, 8, 9)
    local encoded = base32.encode(binary)

    -- Verify it produces valid Base32
    assert(encoded:match("^[A-Z2-7=]+$"),
           "encoded binary should be valid Base32")

    -- Test round trip
    local decoded = base32.decode(encoded)
    assert_eq(decoded, binary, "binary data round trip")

    -- Test all byte values
    local all_bytes = ""
    for i = 0, 255 do
        all_bytes = all_bytes .. string.char(i)
    end
    encoded = base32.encode(all_bytes)
    decoded = base32.decode(encoded)
    assert_eq(decoded, all_bytes, "all byte values round trip")
end)

test("test_rfc_byte_length_patterns", function()
    -- Test different input lengths to verify correct padding
    local length_tests = {
        {
            "a",
            8,
            6,
        }, -- 1 byte -> 8 chars with 6 padding
        {
            "ab",
            8,
            4,
        }, -- 2 bytes -> 8 chars with 4 padding
        {
            "abc",
            8,
            3,
        }, -- 3 bytes -> 8 chars with 3 padding
        {
            "abcd",
            8,
            1,
        }, -- 4 bytes -> 8 chars with 1 padding
        {
            "abcde",
            8,
            0,
        }, -- 5 bytes -> 8 chars with 0 padding
    }

    for _, test_case in ipairs(length_tests) do
        local input, expected_len, expected_padding = test_case[1],
                                                      test_case[2], test_case[3]
        local encoded = base32.encode(input, "rfc")

        assert_eq(string.len(encoded), expected_len,
                  string.format("%d bytes should encode to %d characters",
                                string.len(input), expected_len))

        local padding_count = string.len(encoded) -
                                  string.len(encoded:gsub("=", ""))
        assert_eq(padding_count, expected_padding,
                  string.format("%d bytes should have %d padding chars",
                                string.len(input), expected_padding))

        -- Test round trip
        local decoded = base32.decode(encoded)
        assert_eq(decoded, input,
                  string.format("round trip for %d bytes", string.len(input)))
    end
end)

-- Crockford Base32 Tests

test("test_crockford_basic_functionality", function()
    -- Test basic encoding/decoding
    assert_eq(base32.encode("", "crockford"), "", "empty string encoding")
    assert_eq(base32.decode("", "crockford"), "", "empty string decoding")

    -- Test no padding
    local result = base32.encode("f", "crockford")
    assert(not result:match("="), "Crockford Base32 should not use padding")
    assert_eq(result, "CR", "single char encoding")

    -- Test known values
    assert_eq(base32.encode("foobar", "crockford"), "CSQPYRK1E8",
              "foobar encoding")
    assert_eq(base32.decode("CSQPYRK1E8", "crockford"), "foobar",
              "foobar decoding")
end)

test("test_crockford_special_characters", function()
    -- Test case insensitive
    local test_cases = {
        {
            "CSQPYRK1E8",
            "uppercase",
        },
        {
            "csqpyrk1e8",
            "lowercase",
        },
        {
            "CsQpYrK1e8",
            "mixed case",
        },
    }

    for _, case in ipairs(test_cases) do
        local encoded, desc = case[1], case[2]
        local result = base32.decode(encoded, "crockford")
        assert_eq(result, "foobar", desc .. " decode")
    end

    -- Test hyphen handling
    local with_hyphens = base32.decode("CS-QP-YR-K1-E8", "crockford")
    local multiple_hyphens = base32.decode("CS--QP---YRK1E8", "crockford")
    assert_eq(with_hyphens, "foobar", "decode with hyphens")
    assert_eq(multiple_hyphens, "foobar", "decode with multiple hyphens")

    -- Test I/L -> 1 mapping
    local with_i = base32.decode("CSQPYRKIE8", "crockford")
    local with_l = base32.decode("CSQPYRKLE8", "crockford")
    assert_eq(with_i, "foobar", "I maps to 1")
    assert_eq(with_l, "foobar", "L maps to 1")

    -- Test O -> 0 mapping
    local with_o = base32.decode("OSQPYRK1E8", "crockford")
    local with_zero = base32.decode("0SQPYRK1E8", "crockford")
    assert_eq(with_o, with_zero, "O maps to 0")
end)

test("test_crockford_invalid_characters", function()
    -- Test invalid characters
    local invalid_chars = {
        "U",
        "!",
    }
    for _, char in ipairs(invalid_chars) do
        local invalid_string = "CSQPYRK" .. char .. "E8"
        local res, err = base32.decode(invalid_string, "crockford")
        assert(not res,
               string.format("should reject invalid character '%s'", char))
        assert(tostring(err):match("Illegal character.*'" .. char),
               string.format("error should mention invalid character '%s'", char))
    end
end)

test("test_crockford_binary_data", function()
    -- Test binary data round trip
    local binary = string.char(0, 1, 2, 3, 4, 5, 6, 7, 8, 9)
    local encoded = base32.encode(binary, "crockford")
    local decoded = base32.decode(encoded, "crockford")
    assert_eq(decoded, binary, "Crockford binary data round trip")

    -- Test longer string
    local long_str = string.rep("0123456789", 100)
    encoded = base32.encode(long_str, "crockford")
    decoded = base32.decode(encoded, "crockford")
    assert_eq(decoded, long_str, "long string Crockford round trip")
end)

-- General functionality tests

test("test_format_handling", function()
    -- Test default format
    local rfc_explicit = base32.encode("test", "rfc")
    local rfc_default = base32.encode("test")
    assert_eq(rfc_default, rfc_explicit, "default format should be RFC")

    local decoded_default = base32.decode(rfc_explicit)
    local decoded_explicit = base32.decode(rfc_explicit, "rfc")
    assert_eq(decoded_default, decoded_explicit,
              "default decode format should be RFC")

    -- Test invalid format
    local invalid_formats = {
        "invalid",
        "base64",
        "hex",
    }
    for _, format in ipairs(invalid_formats) do
        local ok_encode, err_encode = pcall(base32.encode, "test", format)
        local ok_decode, err_decode = pcall(base32.decode, "TEST", format)

        assert(not ok_encode, string.format(
                   "should reject invalid encode format '%s'", format))
        assert(not ok_decode, string.format(
                   "should reject invalid decode format '%s'", format))
        assert(err_encode:match("option") or err_encode:match("invalid"),
               "encode error should mention invalid option")
        assert(err_decode:match("option") or err_decode:match("invalid"),
               "decode error should mention invalid option")
    end
end)

test("test_argument_validation", function()
    -- Test non-string argument cannot be decoded
    local ok, err = pcall(base32.decode, 123)
    assert(not ok, "should fail with non-string decode argument")
    assert(err:match("string expected, got number"),
           "error should mention string expected for decode")
end)

test("test_comprehensive_round_trips", function()
    -- Test various string lengths
    local test_strings = {
        "",
        "a",
        "hello",
        "Hello, World!",
        string.rep("abcdefghijklmnopqrstuvwxyz", 10),
    }

    for _, original in ipairs(test_strings) do
        -- RFC round trip
        local rfc_encoded = base32.encode(original, "rfc")
        local rfc_decoded = base32.decode(rfc_encoded, "rfc")
        assert_eq(rfc_decoded, original,
                  string.format("RFC round trip for: %q", original))

        -- Crockford round trip
        local crockford_encoded = base32.encode(original, "crockford")
        local crockford_decoded = base32.decode(crockford_encoded, "crockford")
        assert_eq(crockford_decoded, original,
                  string.format("Crockford round trip for: %q", original))
    end
end)

-- Run all tests
run_tests()
