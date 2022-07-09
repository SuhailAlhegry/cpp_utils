#if !defined(ACHILLES_DEFER_HPP)
#define ACHILLES_DEFER_HPP

template<typename F>
struct defer_t {
    F f;
    defer_t(F &&f) : f(static_cast<F &&>(f)) {}
    ~defer_t() {
        f();
    }
};

template<typename F>
inline defer_t<F> defer_f(F &&f) {
    return defer_t<F>(static_cast<F &&>(f));
}

#if !defined(macro_concat)
    #define macro_concat(x, y) x##y
    #define macro_concat2(x, y) macro_concat(x, y)
#endif
#define defer(code) auto macro_concat2(d_defer_, __COUNTER__) = defer_f([&]() -> void { code;})

#endif

