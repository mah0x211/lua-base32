/**
 *  Copyright 2025 Masatoshi Fukunaga. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

#include "lua_errno.h"
#include <lauxlib.h>
#include <lua.h>
// include system headers
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Check if the argument at index `arg` is a string and return its value.
 *
 * @param L Lua state
 * @param arg Argument index
 * @param len Pointer to store the length of the string
 * @return const char* Pointer to the string value
 */
static inline const uint8_t *checklbytes(lua_State *L, int arg, size_t *len)
{
    luaL_checktype(L, arg, LUA_TSTRING);
    return (const uint8_t *)lua_tolstring(L, arg, len);
}

// RFC 4648 Base32 Decoding table
// https://datatracker.ietf.org/doc/html/rfc4648#section-6
static const uint8_t RFC_DECODE_TABLE[256] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF,

    // 2-7
    26, 27, 28, 29, 30, 31,

    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    // A-Z
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25,

    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    // a-z
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25,

    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF};

// Crockford's Base32 Decoding table (0xFF means invalid character)
static const uint8_t CROCKFORD_DECODE_TABLE[256] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    // 0-9
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,

    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    // A-Z (I=1, L=1, O=0, U=0xFF)
    10, 11, 12, 13, 14, 15, 16, 17, 1, 18, 19, 1, 20, 21, 0, 22, 23, 24, 25, 26,
    0xFF, 27, 28, 29, 30, 31,

    //
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    // a-z (i=1, l=1, o=0, u=0xFF)
    10, 11, 12, 13, 14, 15, 16, 17, 1, 18, 19, 1, 20, 21, 0, 22, 23, 24, 25, 26,
    0xFF, 27, 28, 29, 30, 31,

    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, -1};

static int decode_lua(lua_State *L)
{
    size_t len               = 0;
    const uint8_t *src       = checklbytes(L, 1, &len);
    const char *const opts[] = {"rfc", "crockford", NULL};
    int opt                  = luaL_checkoption(L, 2, "rfc", opts);
    const uint8_t *tbl       = NULL;
    uint64_t acc             = 0;
    int nbits                = 0;
    luaL_Buffer b            = {0};

    // If the input string is empty, return empty string
    if (len == 0) {
        lua_pushliteral(L, "");
        return 1;
    }

    // reset errno for error handling
    errno = 0;

    // select lookup table based on format
    switch (opt) {
    case 0: { // RFC 4648 Base32
        tbl = RFC_DECODE_TABLE;
        // RFC 4648 Base32 requires input length to be a multiple of 8
        if (len % 8 != 0) {
            lua_pushnil(L);
            errno = EINVAL;
            lua_errno_new_with_message(
                L, errno, "base32.decode",
                "RFC 4648 Base32 requires input length to be a multiple of 8");
            return 2;
        }

        // remove padding characters
        int npad      = 0;
        uint8_t *tail = (uint8_t *)src + len;
        for (; tail > src && tail[-1] == '='; --tail) {
            len--;
            npad++;
            if (npad > 6) {
                // RFC 4648 Base32 allows at most 6 padding characters
                lua_pushnil(L);
                errno = EINVAL;
                lua_errno_new_with_message(
                    L, errno, "base32.decode",
                    "RFC 4648 Base32 padding length must be 0, 1, 3, 4, or 6");
                return 2;
            }
        }
        // number of padding characters must be 0, 1, 3, 4, or 6
        if (npad != 0 && npad != 1 && npad != 3 && npad != 4 && npad != 6) {
            lua_pushnil(L);
            errno = EINVAL;
            lua_errno_new_with_message(
                L, errno, "base32.decode",
                "RFC 4648 Base32 padding length must be 0, 1, 3, 4, or 6");
            return 2;
        }
    } break;

    case 1: // crockford
        tbl = CROCKFORD_DECODE_TABLE;
        break;
    }

    // Calculate maximum output size (5 bits per character -> 8 bits per
    // byte) size_t dstlen      = (len * 5 + 7) / 8;
    luaL_buffinit(L, &b);

    for (size_t i = 0; i < len; i++) {
        uint8_t c  = src[i];
        uint8_t dc = tbl[c];

        // In Crockford's Base32, '-' is allowed for readability
        if (c == '-' && tbl == CROCKFORD_DECODE_TABLE) {
            continue;
        }

        // Check if character is valid
        if (dc > 31) {
            char errmsg[256] = {0};
            snprintf(errmsg, sizeof(errmsg),
                     "Illegal character in Base32 string: '%c' (0x%02X) at "
                     "position %d",
                     c, c, (int)(i + 1));
            lua_pushnil(L);
            errno = EILSEQ;
            lua_errno_new_with_message(L, errno, "base32.decode", errmsg);
            return 2;
        }

        // Add 5 bits to buffer
        acc = (acc << 5) | dc;
        nbits += 5;

        // Extract 5 bytes when we have 40 bits
        if (nbits >= 40) {
            // Extract 5 bytes (40 bits)
            char buf[5] = {
                (acc >> 32) & 0xFF, (acc >> 24) & 0xFF, (acc >> 16) & 0xFF,
                (acc >> 8) & 0xFF,  acc & 0xFF,
            };
            luaL_addlstring(&b, buf, 5);
            acc >>= 40;
            nbits -= 40;
        }
    }

    // Handle remaining bits (less than 40 bits)
    while (nbits >= 8) {
        nbits -= 8;
        luaL_addchar(&b, (acc >> nbits) & 0xFF);
    }

    // Push result as Lua string
    luaL_pushresult(&b);
    return 1;
}

// RFC 4648 Base32 encoding/decoding
// https://datatracker.ietf.org/doc/html/rfc4648#section-6
static const char RFC_ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

// Crockford's Base32 alphabet (excluding I, L, O, U)
static const char CROCKFORD_ALPHABET[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

static int encode_lua(lua_State *L)
{
    size_t len               = 0;
    const uint8_t *src       = checklbytes(L, 1, &len);
    const uint8_t *head      = src;
    const uint8_t *tail      = head + len;
    const char *const opts[] = {"rfc", "crockford", NULL};
    int opt                  = luaL_checkoption(L, 2, "rfc", opts);
    const char *tbl          = NULL;
    size_t outlen            = 0;
    luaL_Buffer b            = {0};

    // If the input string is empty, return empty string
    if (len == 0) {
        lua_pushliteral(L, "");
        return 1;
    }

    // select lookup table based on option
    switch (opt) {
    case 0: // RFC 4648 Base32
        tbl = RFC_ALPHABET;
        break;
    case 1: // crockford
        tbl = CROCKFORD_ALPHABET;
        break;
    }

    luaL_buffinit(L, &b);

    // Process 5 bytes at a time (40 bits -> 8 characters)
    while ((tail - head) >= 5) {
        // Load 5 bytes (40 bits)
        uint64_t acc = ((uint64_t)head[0] << 32) | ((uint64_t)head[1] << 24) |
                       ((uint64_t)head[2] << 16) | ((uint64_t)head[3] << 8) |
                       (uint64_t)head[4];
        // Extract 8 characters (5 bits each)
        char buf[8] = {tbl[(acc >> 35) & 0x1F], tbl[(acc >> 30) & 0x1F],
                       tbl[(acc >> 25) & 0x1F], tbl[(acc >> 20) & 0x1F],
                       tbl[(acc >> 15) & 0x1F], tbl[(acc >> 10) & 0x1F],
                       tbl[(acc >> 5) & 0x1F],  tbl[acc & 0x1F]};
        luaL_addlstring(&b, buf, 8);
        outlen += 8; // Increment output length by 8 characters
        head += 5;   // Move source pointer forward by 5 bytes
    }

    // Handle remaining bytes (1-4 bytes)
    if (head < tail) {
        uint32_t acc = 0;
        int nbits    = 0;

        // Load remaining bytes into accumulator
        while (head < tail) {
            acc = (acc << 8) | *head++;
            nbits += 8;
        }
        // Extract all complete 5-bit groups
        while (nbits >= 5) {
            nbits -= 5;
            luaL_addchar(&b, tbl[(acc >> nbits) & 0x1F]);
            outlen++;
        }
        // Handle remaining bits (if any)
        if (nbits > 0) {
            luaL_addchar(&b, tbl[(acc << (5 - nbits)) & 0x1F]);
            outlen++;
        }
    }

    // Add padding for RFC Base32 format
    if (tbl == RFC_ALPHABET && outlen % 8 != 0) {
        // Calculate current length from the data we've written
        static const char PAD[8] = "========";
        size_t padding_len       = 8 - (outlen % 8);
        // Add padding characters
        luaL_addlstring(&b, PAD, padding_len);
    }

    // Null terminate and push result
    luaL_pushresult(&b);
    return 1;
}

LUALIB_API int luaopen_base32(lua_State *L)
{
    // Load errno library for error handling
    lua_errno_loadlib(L);
    // Export the base32 functions
    lua_createtable(L, 0, 2);
    lua_pushcfunction(L, encode_lua);
    lua_setfield(L, -2, "encode");
    lua_pushcfunction(L, decode_lua);
    lua_setfield(L, -2, "decode");
    return 1;
}
