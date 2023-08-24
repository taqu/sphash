/**
# License
This software is distributed under two licenses, choose whichever you like.

## MIT License
Copyright (c) 2023 Takuro Sakai

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Public Domain
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/
#include "sphash.h"
#include <cstring>
#include <intrin.h>

namespace sph
{
namespace
{
    static const uint64_t primes[4] = {0xA0761D6478BD642FULL, 0xE7037ED1A0B428DBULL, 0x8EBC6AF09C88C6E3ULL, 0x589965CC75374CC3ULL};

    inline uint64_t read2(const uint8_t* p)
    {
        return p[0] | (static_cast<uint64_t>(p[1])<<8);
    }

    inline uint64_t read4(const uint8_t* p)
    {
        uint32_t x;
        std::memcpy(&x, p, sizeof(uint32_t));
        return x;
    }

    inline uint64_t read8(const uint8_t* p)
    {
        uint64_t x;
        std::memcpy(&x, p, sizeof(uint64_t));
        return x;
    }

    inline uint64_t rot64(uint64_t x)
    {
        return (x >> 32) | (x << 32);
    }

    inline uint64_t rot64(uint64_t x, uint32_t s)
    {
        return (x >> s) | (x << (64 - s));
    }

    void mul(uint64_t& x0, uint64_t& x1)
    {
#if defined(__SIZEOF_INT128__) || (defined(_INTEGRAL_MAX_BITS) && 128 <= _INTEGRAL_MAX_BITS)
        __uint128_t r = x0;
        r *= x1;
        x0 = (uint64_t)r;
        x1 = (uint64_t)(r >> 64U);
#elif defined(_MSC_VER) && defined(_M_X64)
        x0 = _umul128(x0, x1, &x1);
#else
        uint64_t hh = (x0 >> 32) * (x1 >> 32);
        uint64_t hl = (x0 >> 32) * (uint32_t)x1;
        uint64_t lh = (uint32_t)x0 * (x1 >> 32);
        uint64_t ll = (uint64_t)(uint32_t)x0 * (uint32_t)x1;

        x0 = rot64(hl) ^ hh;
        x1 = rot64(lh) ^ ll;
#endif
    }

    inline uint64_t mix(uint64_t x0, uint64_t x1)
    {
        mul(x0, x1);
        return x0 ^ x1;
    }
} // namespace

uint32_t sphash32(size_t size, const void* data, uint64_t seed)
{
    uint64_t r = sphash64(size, data, seed);
    return static_cast<uint32_t>(r - (r>>32UL));
}

uint64_t sphash64(size_t size, const void* data, uint64_t seed)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    seed ^= primes[0];
    uint64_t x0;
    uint64_t x1;
    if(16 < size) {
        uint64_t s = size;
        if(32 < s) {
            uint64_t seed0 = seed;
            uint64_t seed1 = seed;
            do {
                seed0 = mix(read8(p) ^ primes[2], read8(p + 8) ^ seed0);
                seed1 = mix(read8(p + 16) ^ primes[3], read8(p + 24) ^ seed1);
                p += 32;
                s -= 32;
            } while(32 < s);
            seed = seed0 ^ seed1;
        }
        while(16 < s) {
            seed = mix(read8(p) ^ primes[1], read8(p + 8) ^ seed);
            p += 16;
            s -= 16;
        }
        x0 = read8(p + s - 16);
        x1 = read8(p + s - 8);
    } else {
        if(4 <= size) {
            switch(size) {
            case 4:
                x0 = read2(p);
                x1 = read2(p+2);
                break;
            case 5:
                x0 = read4(p);
                x1 = p[4];
                break;
            case 6:
                x0 = read4(p);
                x1 = p[4] | (static_cast<uint64_t>(p[5])<<8);
                break;
            case 7:
                x0 = read4(p);
                x1 = p[4] | (static_cast<uint64_t>(p[5])<<8) | (static_cast<uint64_t>(p[6])<<16);
                break;
            case 8:
                x0 = read4(p);
                x1 = p[4] | (static_cast<uint64_t>(p[5])<<8) | (static_cast<uint64_t>(p[6])<<16) | (static_cast<uint64_t>(p[7])<<24);
                break;
            case 9:
                x0 = read8(p);
                x1 = p[8];
                break;
            case 10:
                x0 = read8(p);
                x1 = p[8] | (static_cast<uint64_t>(p[9])<<8);
                break;
            case 11:
                x0 = read8(p);
                x1 = p[8] | (static_cast<uint64_t>(p[9])<<8) | (static_cast<uint64_t>(p[10])<<16);
                break;
            case 12:
                x0 = read8(p);
                x1 = read4(p+8);
                break;
            case 13:
                x0 = read8(p);
                x1 = read4(p+8) | (static_cast<uint64_t>(p[12])<<32);
                break;
            case 14:
                x0 = read8(p);
                x1 = read4(p+8) | (static_cast<uint64_t>(p[12])<<32) | (static_cast<uint64_t>(p[13])<<40);
                break;
            case 15:
                x0 = read8(p);
                x1 = read4(p+8) | (static_cast<uint64_t>(p[12])<<32) | (static_cast<uint64_t>(p[13])<<40) | (static_cast<uint64_t>(p[14])<<48);
                break;
            case 16:
                x0 = read8(p);
                x1 = read8(p + 8);
                break;
            default:
                x0 = x1 = 0;
                break;
            }
        } else {
            switch(size) {
            case 1:
                x0 = p[0];
                break;
            case 2:
                x0 = p[0] | (static_cast<uint64_t>(p[1])<<8);
                break;
            case 3:
                x0 = p[0] | (static_cast<uint64_t>(p[1])<<8) | (static_cast<uint64_t>(p[2])<<16);
                break;
            default:
                x0 = 0;
                break;
            }
            x1 = 0;
        }
    }
    return mix(primes[1]^size, mix(x0 ^ primes[1], x1 ^ seed));
}
} // namespace sph
