#if !defined(ACHILLES_ASSERT_HPP)
#define ACHILLES_ASSERT_HPP

#include <stdio.h>

bool aassert_handler(const char *conditionText, const char *report) {
    printf("assertion %s failed, %s \n", conditionText, report);
    return true;
}

#if !defined(RELEASE)
    #define aassert(condition, report) ((void)(!(condition) && aassert_handler(#condition, report) && (exit(1), 1)))
#else
    #define aassert(condition, report) ((void)(condition))
#endif

#endif

