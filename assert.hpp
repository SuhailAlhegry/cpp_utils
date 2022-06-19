#if !defined(ACHILLES_ASSERT_HPP)
#define ACHILLES_ASSERT_HPP

bool aassert_handler(const char *conditionCode, const char *report); 

#if !defined(RELEASE)
    #define aassert(condition, report) ((void)(!(condition) && aassert_handler(#condition, report) && (exit(1), 1)))
#else
    #define aassert(condition, report) ((void)(condition))
#endif

#endif

