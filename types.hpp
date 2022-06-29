#if !defined(ACHILLES_TYPES_HPP)
#define ACHILLES_TYPES_HPP

using u64 = unsigned long long;
using u32 = unsigned int;
using u16 = unsigned short;
using u8  = unsigned char;

using s64 = signed long long;
using s32 = signed int;
using s16 = signed short;
using s8  = signed char;

using f64 = double;
using f32 = float;

#define macro_concat(a, b) a##b
#define macro_concat2(a, b) macro_concat(a, b)

#define Unsigned(nbits) macro_concat2(u, nbits)
#define Signed(nbits) macro_concat2(s, nbits)
#define Float(nbits) macro_concat2(f, nbits)

constexpr u64 U64_MAX = 0xFFFFFFFFFFFFFFFF;
constexpr u32 U32_MAX = 0xFFFFFFFF;
constexpr u16 U16_MAX = 0xFFFF;
constexpr u8  U8_MAX  = 0xFF;

constexpr s64 S64_MAX = 0x7FFFFFFFFFFFFFFF;
constexpr s64 S64_MIN = 0x8000000000000000;
constexpr s32 S32_MAX = 0x7FFFFFFF;
constexpr s32 S32_MIN = 0x80000000;
constexpr s16 S16_MAX = 0x7FFF;
constexpr s16 S16_MIN = 0x8000;
constexpr s8  S8_MAX  = 0x7F;
constexpr s8  S8_MIN  = 0x80;


#endif

