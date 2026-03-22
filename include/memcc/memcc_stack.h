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

alignas(uint64_t) struct memcc_dfmeta {
    uint32_t lcount; // distance in bytes to previous meta & flag collapse
    uint32_t ncount; // distance in bytes to next meta
};

typedef struct memcc_dfstack {
    uint64_t *pool; // start of memory
    uint64_t *last; // points to current top meta
    uint32_t count; // total count of uint64_t in pool
} memcc_dfstack_t;


/* ================================================================================ */
/*  DEFERRED_STACK                                                                  */
/* ================================================================================ */

static inline void memcc_setup_dfstack(memcc_dfstack_t *dfstack, void *pool, size_t size) {
    MEMCC_CHECK(dfstack && pool && size > 0, /*void*/);

    dfstack->pool = (uint64_t *) memcc_align_ptr(pool, alignof(uint64_t));
    dfstack->last = dfstack->pool;

    // not the best, really not the best, but get the count of element
    size_t aligned_size = size - ((uintptr_t)dfstack->pool - (uintptr_t)pool);
    dfstack->count = aligned_size / sizeof(uint64_t);

    MEMCC_ZERO_ALLOC(dfstack->pool, dfstack->count);

    // makes lots of sense if zero_alloc is disabled
    struct memcc_dfmeta *meta = (struct memcc_dfmeta *)dfstack->last;
    meta->lcount = 0;
    meta->ncount = 0;
}
static inline void memcc_teardown_dfstack(memcc_dfstack_t *dfstack) {
    MEMCC_CHECK(dfstack, /*void*/);

    dfstack->pool = NULL;
    dfstack->last = NULL;
    dfstack->count = 0;
}

static inline void *memcc_dfstack_push(memcc_dfstack_t *dfstack, uint32_t size, uint32_t align) {
    MEMCC_CHECK(dfstack && memcc_pow2(align), NULL);

    // calculate payload, span and next
    uint64_t *payload_base = dfstack->last + 1;
    uint64_t *payload = memcc_align_ptr(payload_base, align);

    // not optimal at all
    uint32_t pad = ((uintptr_t)payload - (uintptr_t)payload_base);
    uint32_t span = (pad + size + 7) / sizeof(uint64_t);

    // technically the memcc_align_ptr is not needed, as it is an emergent behavior from the algorithm
    struct memcc_dfmeta *next = memcc_align_ptr(payload + span, sizeof(uint64_t));

    // Bounds check
    if((uint32_t)( next - (struct memcc_dfmeta *)dfstack->pool ) + 1 > dfstack->count)
        return NULL;

    // Move the last to new position
    struct memcc_dfmeta *olast = (struct memcc_dfmeta *)dfstack->last;
    struct memcc_dfmeta *nlast = (struct memcc_dfmeta *)(payload-1);

    if(olast != nlast) {
        // tell the pair that we have moved.
        if(olast->lcount != 0) {
            struct memcc_dfmeta *olast_pair = olast - olast->lcount;
            olast_pair->ncount = nlast - olast_pair;
        }

        nlast->lcount = olast->lcount;
        nlast->ncount = olast->ncount;
    }

    nlast->ncount = next - nlast;
    next->lcount = nlast->ncount;
    next->ncount = 0;

    MEMCC_ZERO_ALLOC(nlast + 1, nlast->ncount-1);

    dfstack->last = (uint64_t *)next;
    return payload;
}
static inline void *memcc_dfstack_top(memcc_dfstack_t *dfstack) {
    MEMCC_CHECK(dfstack && dfstack->pool != dfstack->last, NULL);

    struct memcc_dfmeta *foot = (struct memcc_dfmeta *)dfstack->last;
    struct memcc_dfmeta *head = foot - (foot->lcount & MEMCC_DFSTACK_VALUE);
    
    return (void *)head + 1;
}

static inline void memcc_dfstack_pop(memcc_dfstack_t *dfstack, void *addr) {
    MEMCC_CHECK(dfstack && dfstack->last != dfstack->pool, /*void*/);

    // behavior normal pop, without addr
    struct memcc_dfmeta *foot = (struct memcc_dfmeta *)dfstack->last;
    struct memcc_dfmeta *head = foot - (foot->lcount & MEMCC_DFSTACK_VALUE);

    // behavior with addr
    if(addr) {
        // technically bound check, but it is allowing too high, for now it's ok though
        MEMCC_CHECK((uint64_t *) addr - dfstack->pool > 0 && (uint64_t *) addr - dfstack->pool < dfstack->count, /*void*/);

        head = (struct memcc_dfmeta *)addr - 1;
        struct memcc_dfmeta * nfoot = head + head->ncount;

        // deffered free
        if(nfoot != foot) {
            foot->lcount |= MEMCC_DFSTACK_FLAGS;
            return;
        }

        foot = nfoot;
    }

    // normal behavior otherwise
    foot->lcount = 0;
    foot->ncount = 0;
    dfstack->last = (uint64_t *)head;


    // cascade deffered collapses
    while(dfstack->last != dfstack->pool) {
        struct memcc_dfmeta *prev = (struct memcc_dfmeta *)dfstack->last;

        if((prev->lcount & MEMCC_DFSTACK_FLAGS) == 0)
            break;

        dfstack->last -= prev->lcount & MEMCC_DFSTACK_VALUE;
    }

    MEMCC_ZERO_FREE(dfstack->last+1, head - (struct memcc_dfmeta *)dfstack->last);
}
static inline void memcc_dfstack_clear(memcc_dfstack_t *dfstack) {
    MEMCC_CHECK(dfstack, /*void*/);

    dfstack->last = dfstack->pool;
    MEMCC_ZERO_FREE(dfstack->pool, dfstack->count * sizeof(uint64_t));

    // makes lots of sense if zero_alloc is disabled
    struct memcc_dfmeta *meta = (struct memcc_dfmeta *)dfstack->last;
    meta->lcount = 0;
    meta->ncount = 0;
}


#ifdef __cplusplus
}
#endif

#endif /* MEMCC_STACK_H */