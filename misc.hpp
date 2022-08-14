#if !defined(ACHILLES_MISC_HPP)
#define ACHILLES_MISC_HPP

#if !defined(macro_concat)
    #define macro_concat(x, y) x##y
    #define macro_concat2(x, y) macro_concat(x, y)
#endif

#if defined(__COUNTER__)
    #define ANON_VAL __COUNTER__
#else
    #define ANON_VAL __LINE__
#endif

#define ANON_VAR(NAME) macro_concat2(NAME, ANON_VAL)

#endif

