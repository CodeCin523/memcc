#ifndef MEMCC_STACK_H
#define MEMCC_STACK_H

#ifdef __cplusplus
extern "C" {
#endif


#include "memcc.h"

#include <stdalign.h>


/* ================================================================================ */
/*  TYPEDEF                                                                         */
/* ================================================================================ */

#define MEMCC_DFSTACK_FLAGS (0x80000000u)
#define MEMCC_DFSTACK_VALUE (0x7FFFFFFFu)

struct memcc_dfmeta {
    uint32_t lcount; // distance in sizeof(memcc_dfmeta) to previous meta & flag collapse
    uint32_t ncount; // distance in sizeof(memcc_dfmeta) to next meta
};

typedef struct memcc_dfstack {
    uint8_t *pool;  // start of memory
    uint8_t *last;  // points to current top meta
    uint32_t size;  // total byte count in pool
} memcc_dfstack_t;


/* ================================================================================ */
/*  DEFERRED_STACK                                                                  */
/* ================================================================================ */

static inline void memcc_setup_dfstack_tu(memcc_dfstack_t *dfstack, void *pool, size_t size) {
    MEMCC_CHECK(dfstack && pool && size > 0, /*void*/);

    dfstack->pool = (uint8_t *) memcc_align_ceil(pool, alignof(max_align_t));
    dfstack->last = dfstack->pool;

    // get size, ajust with moved pool
    dfstack->size = size - ((uintptr_t)dfstack->pool - (uintptr_t)pool);

    MEMCC_ZERO_ALLOC(dfstack->pool, dfstack->size);

#ifndef MEMCC_ZERO_ON_ALLOC
    struct memcc_dfmeta *meta = (struct memcc_dfmeta *)dfstack->last;
    meta->lcount = 0;
    meta->ncount = 0;
#endif
}
static inline void memcc_teardown_dfstack_tu(memcc_dfstack_t *dfstack) {
    MEMCC_CHECK(dfstack, /*void*/);

    dfstack->pool = NULL;
    dfstack->last = NULL;
    dfstack->size = 0;
}

static inline void *memcc_dfstack_push_tu(memcc_dfstack_t *dfstack, uint32_t size, uint32_t align) {
    MEMCC_CHECK(dfstack && memcc_pow2(align), NULL);

    // calculate payload, span and next
    uint8_t *payload = (uint8_t *) memcc_align_ceil(
        dfstack->last + sizeof(struct memcc_dfmeta),
        align
    );

    // where is next
    struct memcc_dfmeta *next = (struct memcc_dfmeta *) memcc_align_ceil(payload + size, alignof(struct memcc_dfmeta));
    // where are lasts
    struct memcc_dfmeta *olast = (struct memcc_dfmeta *) dfstack->last;
    struct memcc_dfmeta *nlast = (struct memcc_dfmeta *) memcc_align_floor(
        payload - sizeof(struct memcc_dfmeta),
        alignof(struct memcc_dfmeta)
    );


    // Bounds check
    if((uint8_t *)(next + 1) - dfstack->pool > dfstack->size)
        return NULL;

    if(olast != nlast) {
        // tell the pair that we have moved.
        if(olast->lcount != 0) {
            struct memcc_dfmeta *olast_pair = olast - (olast->lcount & MEMCC_DFSTACK_VALUE);
            olast_pair->ncount = nlast - olast_pair;
        }

        nlast->lcount = olast->lcount;
        nlast->ncount = olast->ncount;
    }

    nlast->ncount = next - nlast;
    next->lcount = nlast->ncount;
    next->ncount = 0;

    MEMCC_ZERO_ALLOC(nlast+1, (nlast->ncount-1)*sizeof(struct memcc_dfmeta));

    dfstack->last = (uint8_t *)next;
    return payload;
}

static inline void memcc_dfstack_pop_tu(memcc_dfstack_t *dfstack, void *addr) {
    MEMCC_CHECK(dfstack && dfstack->last != dfstack->pool, /*void*/);

    // behavior normal pop, without addr
    struct memcc_dfmeta *foot = (struct memcc_dfmeta *)dfstack->last;
    struct memcc_dfmeta *head = foot - (foot->lcount & MEMCC_DFSTACK_VALUE);

    // behavior with addr
    if(addr) {
        // technically bound check, but it is allowing too high, for now it's ok though
        // MEMCC_CHECK((uint8_t *)addr - dfstack->pool > 0 && (uint8_t *)addr - dfstack->pool < dfstack->size, /*void*/);

        head = (struct memcc_dfmeta *) memcc_align_floor(
            (uint8_t *)addr - sizeof(struct memcc_dfmeta),
            alignof(struct memcc_dfmeta)
        );
        struct memcc_dfmeta * nfoot = head + head->ncount;

        // deffered free
        if(nfoot != foot) {
            foot->ncount |= MEMCC_DFSTACK_FLAGS;
            return;
        }

        foot = nfoot;
    }

    // normal behavior otherwise
    foot->lcount = 0;
    foot->ncount = 0;
    dfstack->last = (uint8_t *)head;


    // cascade deffered collapses
    while(dfstack->last != dfstack->pool) {
        struct memcc_dfmeta *prev = (struct memcc_dfmeta *)dfstack->last;

        if((prev->lcount & MEMCC_DFSTACK_FLAGS) == 0)
            break;

        dfstack->last -= prev->lcount & MEMCC_DFSTACK_VALUE;
    }

    MEMCC_ZERO_FREE(
        dfstack->last+1,
        (uint8_t *)head - dfstack->last
    );
}
static inline void memcc_dfstack_clear_tu(memcc_dfstack_t *dfstack) {
    MEMCC_CHECK(dfstack, /*void*/);

    dfstack->last = dfstack->pool;
    MEMCC_ZERO_ALLOC(dfstack->pool, dfstack->size);

#ifndef MEMCC_ZERO_ON_ALLOC
    struct memcc_dfmeta *meta = (struct memcc_dfmeta *)dfstack->last;
    meta->lcount = 0;
    meta->ncount = 0;
#endif
}


/* ================================================================================ */
/*  INTERFACE ALIAS                                                                 */
/* ================================================================================ */

#define memcc_setup_dfstack(...) memcc_setup_dfstack_tu(__VA_ARGS__)
#define memcc_teardown_dfstack(...) memcc_teardown_dfstack_tu(__VA_ARGS__)

#define memcc_dfstack_push(dfstack, size) memcc_dfstack_push_tu(dfstack, size, alignof(max_align_t))
#define memcc_dfstack_push_align(dfstack, size, align) memcc_dfstack_push_tu(dfstack, size, align)
#define memcc_dfstack_push_type(dfstack, count, type) memcc_dfstack_push_tu(dfstack, sizeof(type)*count, alignof(type))

#define memcc_dfstack_pop(dfstack) memcc_dfstack_pop_tu(dfstack, NULL)
#define memcc_dfstack_pop_addr(dfstack, addr) memcc_dfstack_pop_tu(dfstack, addr)
#define memcc_dfstack_clear(dfstack) memcc_dfstack_clear_tu(dfstack)


#ifdef __cplusplus
}
#endif

#endif /* MEMCC_STACK_H */