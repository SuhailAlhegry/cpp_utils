#if !defined(ACHILLES_ASSERT_HPP)
#define ACHILLES_ASSERT_HPP

inline bool aassert_handler(const char *file, int line, const char *conditionCode, const char *report); 

#if !defined(RELEASE) || defined(RELEASE_ASSERTS)
    #define aassert(condition, report) ((void)(!(condition) && aassert_handler(__FILE__, __LINE__, #condition, report) && (exit(1), 1)))
#else
    #define aassert(condition, report) ((void)(condition))
#endif

#endif

