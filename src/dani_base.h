// Danilib - dani_base.h
// A collection of base types and helper macros to use in a C project.
//
// Author: Dani Drywa (dani@drywa.me)
//
// Last change: 2024/09/19 (yyyy/mm/dd)
//
// License: See end of file
//
#ifndef __DANI_LIB_BASE_H
#define __DANI_LIB_BASE_H

// Basic integer typedefs
typedef signed __int8 s8;
typedef signed __int16 s16;
typedef signed __int32 s32;
typedef signed __int64 s64;

typedef unsigned __int8 u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

// Known integer values
#define S8_MIN (-128)
#define S16_MIN (-32768)
#define S32_MIN (-2147483647l - 1)
#define S64_MIN (-9223372036854775807ll - 1)

#define S8_MAX (127)
#define S16_MAX (32767)
#define S32_MAX (2147483647l)
#define S64_MAX (9223372036854775807ll)

#define U8_MIN (0)
#define U16_MIN (0u)
#define U32_MIN (0ul)
#define U64_MIN (0ull)

#define U8_MAX (0xFF)
#define U16_MAX (0xFFFF)
#define U32_MAX (0xFFFFFFFFul)
#define U64_MAX (0xFFFFFFFFFFFFFFFFull)

// Basic floating point typedefs
typedef float f32;
typedef double f64;

// Known floating point values
#define F32_MAX (3.402823466e+38f)
#define F64_MAX (1.7976931348623158e+308)

// Basic boolean typedefs
typedef u32 b32;

// Known boolean values
#define B32_TRUE (1ul)
#define B32_FALSE (0ul)

#define B32_SUCCESS B32_TRUE
#define B32_FAILURE B32_FALSE

// Boolean helper macros
#define IsTrue(x) ((x) != B32_FALSE)
#define IsFalse(x) ((x) == B32_FALSE)

#define IsSuccess(x) ((x) != B32_FALSE)
#define IsFailure(x) ((x) == B32_FALSE)

// Unit conversion macros
#define KiB(kib) (((u64)(kib)) << 10)
#define MiB(mib) (((u64)(mib)) << 20)
#define GiB(gib) (((u64)(gib)) << 30)
#define TiB(tib) (((u64)(tib)) << 40)

#define Kilo(kilo) ((kilo) * 1000)
#define Mega(mega) (Kilo(mega) * 1000)
#define Giga(giga) (Mega(giga) * 1000)
#define Tera(tera) (Giga(tera) * 1000)

#define Thousand(thousand) Kilo(thousand)
#define Million(million) Mega(million)
#define Billion(billion) Giga(billion)
#define Trillion(trillion) Tera(trillion)

// Assert macros
#define Statement(x) do { x } while(0)
#define Trap() __debugbreak()
#define AssertAlways(x) Statement(if (!(x)) { Trap(); })
#ifndef NDEBUG
    #define Assert(x) AssertAlways(x)
#else
    #define Assert(x) (void)(x)
#endif
#define NotImplemented Assert(!"Not Implemented!")

// General helper macros
#define Unused(x) ((void)(x))
#define ArrayCount(x) (sizeof(x) / sizeof((x)[0]))

// String helper macros
#define StringifyNoExpand(x) #x
#define Stringify(x) StringifyNoExpand(x)

#define StringifyCombineNoExpand(a, b) a##b
#define StringifyCombine(a, b) StringifyCombineNoExpand(a, b)

// Math helper macros
#define Abs(x) (((x) >= 0) ? (x) : -(x))
#define Min(a, b) (((a) < (b)) ? (a) : (b))
#define Max(a, b) (((a) > (b)) ? (a) : (b))

#define ClampCeiling(x, ceiling) Min(x, ceiling)
#define ClampFloor(x, floor) Max(x, floor)
#define Clamp(x, floor, ceiling) ClampCeiling(ClampFloor(x, floor), ceiling)

#define IsPower2(x) ((((x) - 1) & (x)) == 0)
#define AlignPower2(x, alignment) (((x) + (alignment) - 1) & (~((alignment) - 1)))

// Bit Helper macros
#define ByteSplat16(x) (((~(u16)U16_MIN / (u16)255) * (u16)(x)))
#define ByteSplat32(x) (((~U32_MIN) / 255ul) * (x))
#define ByteSplat64(x) (((~U64_MIN) / 255ull) * (x))

#endif // __DANI_LIB_BASE_H

/*
Danilib - dani_base.h License:
---------------------------------------------------------------------------------
Copyright 2024 Dani Drywa (dani@drywa.me)

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

4. If the source or binary form was of value to you, consider donating to the creator at https://ko-fi.com/danicrunch

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------------
*/
