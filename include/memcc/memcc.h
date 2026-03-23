#ifndef MEMCC_H
#define MEMCC_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#if !defined(__GNUC__) && !defined(__clang__)
#include <string.h>
#endif


/* ================================================================================ */
/*  API RULES                                                                       */
/* ================================================================================ */

/*naming convention
 * ├─ case
 * |  ├─ snake_case               : functions, types, variables
 * |  └─ SCREAMING_SNAKE_CASE     : constants, macros, enum values
 * ├─ prefix
 * |  └─ memcc_                   : yes.
 * ├─ function suffix
 * ├─ function suffix
 * |  ├─ tb                     : thread-blocking
 * |  ├─ ts                     : thread-safe
 * |  ├─ tu                     : thread-unsafe
 * └─ type suffix
 *    └─ _t                       : yes, for type definitions
*/


/* ================================================================================ */
/*  MACROS                                                                          */
/* ================================================================================ */

// Check macro
#if defined(MEMCC_NO_CHECKS)
    #define MEMCC_CHECK(expr, ret) ((void)0)
#elif defined(MEMCC_DEBUG)
    #include <assert.h>
    #define MEMCC_CHECK(expr, ret) assert(expr)
#else
    #define MEMCC_CHECK(expr, ret) if (!(expr)) return ret
#endif

// Zero-on-alloc macro
#ifdef MEMCC_ZERO_ON_ALLOC
#define MEMCC_ZERO_ALLOC(ptr, size) memcc_zero(ptr, size)
#else
#define MEMCC_ZERO_ALLOC(ptr, size) ((void)0)
#endif

// Zero-on-free macro
#ifdef MEMCC_ZERO_ON_FREE
#define MEMCC_ZERO_FREE(ptr, size) memcc_zero(ptr, size)
#else
#define MEMCC_ZERO_FREE(ptr, size) ((void)0)
#endif

// Convenience memory sizes
#define MEMCC_KB(b) ((size_t)(b) * 1024UL)
#define MEMCC_MB(b) ((size_t)(b) * 1024UL * 1024UL)
#define MEMCC_GB(b) ((size_t)(b) * 1024UL * 1024UL * 1024UL)


/* ================================================================================ */
/*  MEMORY UTILITIES                                                                */
/* ================================================================================ */

/**
 * Check if a value is a power-of-2 (>0).
 * Returns 1 if true, 0 if false.
 */
static inline int memcc_pow2_ts(uintptr_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

/**
 * Check if value is a multiple of "mul".
 * Returns 1 if true, 0 if false.
 * 'mul' must be a power of 2.
 */
static inline int memcc_pow_ts(uintptr_t value, size_t mul) {
    MEMCC_CHECK(memcc_pow2_ts(mul), 0); // must be power of 2
    return (value & (mul - 1)) == 0;
}

/**
 * Align a pointer or size UP to the next multiple of `align`.
 * Returns the next multiple of `align` >= value.
 * `align` must be >0 and a power-of-2.
 * Returns 0 for invalid input (size) or NULL (pointer).
 */
static inline uintptr_t memcc_align_ceil_ts(uintptr_t value, size_t align) {
    MEMCC_CHECK(memcc_pow2_ts(align), 0); // align must be power of 2
    return (value + (align - 1)) & ~(align - 1);
}

/**
 * Align a pointer or size DOWN to the previous multiple of `align`.
 * Returns the last multiple of `align` <= value.
 * `align` must be >0 and a power-of-2.
 * Returns 0 for invalid input (size) or NULL (pointer).
 */
static inline uintptr_t memcc_align_floor_ts(uintptr_t value, size_t align) {
    MEMCC_CHECK(memcc_pow2_ts(align), 0); // align must be power of 2
    return value & ~(align - 1);
}

/**
 * Clamp value between min and max.
 */
static inline size_t memcc_clamp_ts(size_t value, size_t min, size_t max) {
    const size_t t = value < min ? min : value;
    return t > max ? max : t;
}

/**
 * Set memory to a specific byte value.
 * Optimized for GCC/Clang using __builtin_memset.
 */
static inline void memcc_set_tu(void *ptr, unsigned long value, size_t size) {
    MEMCC_CHECK(ptr != NULL, /*void*/);

#if defined(__GNUC__) || defined(__clang__)
    __builtin_memset(ptr, value, size);
#else
    memset(ptr, value, size);
#endif
}

/**
 * Zero memory region.
 */
static inline void memcc_zero_tu(void *ptr, size_t size) {
    memcc_set_tu(ptr, 0, size);
}


/* ================================================================================ */
/*  INTERFACE ALIAS                                                                 */
/* ================================================================================ */

#define memcc_pow2(value)               memcc_pow2_ts((uintptr_t)(value))
#define memcc_pow(value, mul)           memcc_pow_ts((uintptr_t)(value), (size_t)(mul))

#define memcc_align_ceil(value, align)  memcc_align_ceil_ts((uintptr_t)(value), (size_t)(align))
#define memcc_align_floor(value, align) memcc_align_floor_ts((uintptr_t)(value), (size_t)(align))

#define memcc_clamp(value, min, max)    memcc_clamp_ts((size_t)(value), (size_t)(min), (size_t)(max))
#define memcc_set(ptr, value, size)     memcc_set_tu((void *)(ptr), (unsigned long)(value), (size_t)(size))
#define memcc_zero(ptr, size)           memcc_zero_tu((void *)(ptr), (size_t)(size))


#ifdef __cplusplus
}
#endif

#endif /* MEMCC_H */