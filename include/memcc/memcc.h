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
static inline int memcc_pow2(uintptr_t v) {
    return v != 0 && (v & (v - 1)) == 0;
}

/**
 * Check if value is a multiple of "mul".
 * Returns 1 if true, 0 if false.
 * 'mul' must be a power of 2.
 */
static inline int memcc_pow(uintptr_t value, uintptr_t mul) {
    MEMCC_CHECK(memcc_pow2(mul), 0); // must be power of 2
    return (value & (mul - 1)) == 0;
}

/**
 * Align a pointer forward to the next multiple of `align`.
 * Align must be >0 and power-of-2.
 */
static inline void *memcc_align_forward(void *ptr, size_t align) {
    MEMCC_CHECK(ptr != NULL, NULL);
    MEMCC_CHECK(memcc_pow2(align), NULL);

    uintptr_t p = (uintptr_t)ptr;
    uintptr_t aligned = (p + (align - 1)) & ~(align - 1);
    return (void *)aligned;
}

/**
 * Clamp value between min and max.
 */
static inline size_t memcc_clamp(size_t value, size_t min, size_t max) {
    const size_t t = value < min ? min : value;
    return t > max ? max : t;
}

/**
 * Set memory to a specific byte value.
 * Optimized for GCC/Clang using __builtin_memset.
 */
static inline void memcc_set(void *ptr, int value, size_t size) {
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
static inline void memcc_zero(void *ptr, size_t size) {
    memcc_set(ptr, 0, size);
}


#ifdef __cplusplus
}
#endif

#endif /* MEMCC_H */