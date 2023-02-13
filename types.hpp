#if !defined(ACHILLES_TYPES_HPP)
#define ACHILLES_TYPES_HPP

#include <type_traits>
#include "misc.hpp"

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

#define Unsigned(nbits) macro_concat2(u, nbits)
#define Signed(nbits) macro_concat2(s, nbits)
#define Float(nbits) macro_concat2(f, nbits)

constexpr u64 U64_MAX = 0xFFFFFFFFFFFFFFFF;
constexpr u32 U32_MAX = 0xFFFFFFFF;
constexpr u16 U16_MAX = 0xFFFF;
constexpr u8  U8_MAX  = 0xFF;

constexpr s64 S64_MAX =  0x7FFFFFFFFFFFFFFF;
constexpr s64 S64_MIN = -0x8000000000000000;
constexpr s32 S32_MAX =  0x7FFFFFFF;
constexpr s32 S32_MIN = -0x80000000;
constexpr s16 S16_MAX =  0x7FFF;
constexpr s16 S16_MIN = -0x8000;
constexpr s8  S8_MAX  =  0x7F;
constexpr s8  S8_MIN  = -0x80;

namespace achilles {
    namespace types {
        using TypeHash = u64;

        template<typename T>
        struct remove_all {
            using unref = std::remove_reference_t<T>;
            using type = std::conditional_t<
                    std::is_array_v<unref>,
                    std::remove_all_extents_t<unref> *,
                    unref
                >;
        };

        template<typename T>
        using remove_all_t = typename remove_all<T>::type;

        template<typename T>
        constexpr auto typeName() {
            #if defined(_MSC_VER)
                #define F __FUNCSIG__
            #else
                #define F __PRETTY_FUNCTION__
            #endif
            return F;
        }

        // FNV-1a hash
        template<typename T>
        constexpr u64 typeHash() {
            #if defined(_MSC_VER)
                #define F __FUNCSIG__
            #else
                #define F __PRETTY_FUNCTION__
            #endif
            u64 prime = 0x00000100000001B3;
            u64 offset = 0xcbf29ce484222325;
            u64 hash = offset;
            for (auto const c : F) {
                hash ^= c;
                hash *= prime;
            }
            return hash;
            #undef F
        }

        template<typename T>
        static constexpr auto typehash = typeHash<remove_all_t<T>>();

        template<typename T>
        static constexpr auto type_name = typeName<remove_all_t<T>>();

        struct Any {
            Any() : _ptr{nullptr} {}

            template<typename T>
            Any(T const &v) : _type{typehash<T>}, _ptr{(void *) &v} {}
            template<typename T>
            Any(T &&v) : _type{typehash<T>}, _ptr{&v} {}

            template<typename T>
            T value() {
                if constexpr (std::is_function_v<std::remove_pointer_t<T>>) {
                    return (std::remove_pointer_t<T> *) _ptr;
                } else if constexpr (std::is_pointer_v<T> || std::is_member_pointer_v<T>) {
                    return (std::remove_pointer_t<T> *) _ptr;
                } else {
                    return *((T *) _ptr);
                }
            }

            TypeHash type() const {
                return _type;
            }
        private:
            TypeHash _type;
            void *_ptr;
        };
    };
};


#endif

