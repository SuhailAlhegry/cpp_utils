#if !defined(ACHILLES_DEFER_HPP)
#define ACHILLES_DEFER_HPP

#include "misc.hpp"

namespace achilles {
    namespace defer {
        enum class dummy {};

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

        template<typename F>
        inline defer_t<F> operator+(dummy, F &&f) {
            return defer_t<F>(static_cast<F &&>(f));
        }
    };
};

#define defer auto ANON_VAR(DEFER_T) = achilles::defer::dummy() + [&]()

#endif

