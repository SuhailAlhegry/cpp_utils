#if !defined(ACHILLES_ENUMS_HPP)
#define ACHILLES_ENUMS_HPP

#define _MACRO_PARENS ()

#define _MACRO_EXPAND0(arg) arg
#define _MACRO_EXPAND1(arg) _MACRO_EXPAND0(_MACRO_EXPAND0(_MACRO_EXPAND0(_MACRO_EXPAND0(arg))))
#define _MACRO_EXPAND2(arg) _MACRO_EXPAND1(_MACRO_EXPAND1(_MACRO_EXPAND1(_MACRO_EXPAND1(arg))))
#define _MACRO_EXPAND3(arg) _MACRO_EXPAND2(_MACRO_EXPAND2(_MACRO_EXPAND2(_MACRO_EXPAND2(arg))))
#define _MACRO_EXPAND4(arg) _MACRO_EXPAND3(_MACRO_EXPAND3(_MACRO_EXPAND3(_MACRO_EXPAND3(arg))))

#define _MACRO_FOREACH_HELPER0() _MACRO_FOREACH_HELPER1

#define _MACRO_FOREACH_HELPER1(macro, arg0, ...)\
    macro(arg0)\
    __VA_OPT__(_MACRO_FOREACH_HELPER0 _MACRO_PARENS (macro, __VA_ARGS__))

#define _MACRO_FOREACH(macro, ...)\
    __VA_OPT__(_MACRO_EXPAND4(_MACRO_FOREACH_HELPER1(macro, __VA_ARGS__)))

#define _MACRO_ENUM_CASE(name) case name: return #name;

#define _MACRO_ENUM_BEGIN(name)\
    struct name {\
        enum Value {

#define _MACRO_ENUM_BEGIN_TYPED(name, type)\
    struct name {\
        enum Value : type {

#define _MACRO_ENUM_END(name, ...)\
            __VA_ARGS__\
        };\
        name() = default;\
        constexpr name(Value value) : _value(value) {}\
        explicit operator bool() const = delete;\
        constexpr operator Value() const { return _value; }\
        constexpr char const *toString() const {\
            switch (_value) {\
                _MACRO_FOREACH(_MACRO_ENUM_CASE, __VA_ARGS__)\
            };\
            return "";\
        }\
    private:\
        Value _value;\
    }

#define ENUM(name, ...)\
        _MACRO_ENUM_BEGIN(name)\
        _MACRO_ENUM_END(name, __VA_ARGS__)

#define ENUM_T(name, type, ...)\
        _MACRO_ENUM_BEGIN_TYPED(name, type)\
        _MACRO_ENUM_END(name, __VA_ARGS__)

#endif

