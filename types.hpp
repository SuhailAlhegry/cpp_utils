#if !defined(ACHILLES_TYPES_HPP)
#define ACHILLES_TYPES_HPP

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using s8 = signed char;
using s16 = signed short;
using s32 = signed int;
using s64 = signed long long;

using f32 = float;
using f64 = double;

#define macro_concat(a, b) a##b
#define macro_concat2(a, b) macro_concat(a, b)

#define Unsigned(nbits) macro_concat2(u, nbits)
#define Signed(nbits) macro_concat2(s, nbits)
#define Float(nbits) macro_concat2(f, nbits)

#endif

