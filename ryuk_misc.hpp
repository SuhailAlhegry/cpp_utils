#ifndef RYUK_MISC_H
#define RYUK_MISC_H

#include <stdio.h>
#include <utility>

// utils
#define between(a, min, max) ((a) >= (min) && (a) <= (max))

inline bool assert_handler(const char *report, const char *conditionCode, const char *file, int line) {
    printf("assertion raised: '%s', '%s' in '%s' at line %i failed\n", report, conditionCode, file, line);
    return true;
}

#define halt() (exit(1))

#ifdef DEBUG
#define rassert(condition, report) ((void)(!(condition) && assert_handler(report, #condition, __FILE__, __LINE__) && (halt(), 1)))
#else
#define rassert(condition, report) ((void)sizeof(condition))
#endif

template<typename F>
struct _Defer {
    F f;

    _Defer(F &&f) : f(std::forward<F>(f)) {}

    ~_Defer() {
        f();
    }  
};

template<typename F>
inline _Defer<F> _defer_func(F &&f) {
    return _Defer<F>(std::forward<F>(f));
}

#define macro_concat(x, y) x##y
#define macro_concat2(x, y) macro_concat(x, y)
#define defer(code) auto macro_concat2(_defer_, __COUNTER__) = _defer_func([&]()->void{ code; })

#endif
