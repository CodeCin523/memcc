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

static inline void memcc_setup_dfstack_sfy(memcc_dfstack_t *dfstack, void *pool, size_t size) {
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
static inline void memcc_teardown_dfstack_ufy(memcc_dfstack_t *dfstack) {
    MEMCC_CHECK(dfstack, /*void*/);

    dfstack->pool = NULL;
    dfstack->last = NULL;
    dfstack->size = 0;
}

static inline void *memcc_dfstack_push_ufy(memcc_dfstack_t *dfstack, uint32_t size, uint32_t align) {
    MEMCC_CHECK(dfstack && memcc_pow2(align), NULL);

    // calculate payload, span and next
    uint8_t *payload = (uint8_t *) memcc_align_floor(
        dfstack->last + sizeof(struct memcc_dfmeta),
        align
    );

    // where is next
    struct memcc_dfmeta *next = (struct memcc_dfmeta *) memcc_align_ceil(payload + size, alignof(struct memcc_dfmeta));
    // where are lasts
    struct memcc_dfmeta *olast = (struct memcc_dfmeta *) dfstack->last;
    struct memcc_dfmeta *nlast = (struct memcc_dfmeta *) memcc_align_floor(payload - 1, alignof(struct memcc_dfmeta));


    // Bounds check
    if((uint8_t *)(next + 1) - dfstack->pool > dfstack->size)
        return NULL;

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

    MEMCC_ZERO_ALLOC((nlast+1), (nlast->ncount-1)*sizeof(struct memcc_dfmeta));

    dfstack->last = (uint8_t *)next;
    return payload;
}

static inline void memcc_dfstack_pop_umx(memcc_dfstack_t *dfstack, void *addr) {
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
static inline void memcc_dfstack_clear_ufy(memcc_dfstack_t *dfstack) {
    MEMCC_CHECK(dfstack, /*void*/);

    dfstack->last = dfstack->pool;
    MEMCC_ZERO_FREE(dfstack->pool, dfstack->count * sizeof(uint64_t));

    // makes lots of sense if zero_alloc is disabled
    struct memcc_dfmeta *meta = (struct memcc_dfmeta *)dfstack->last;
    meta->lcount = 0;
    meta->ncount = 0;
}


/* ================================================================================ */
/*  INTERFACE ALIAS                                                                 */
/* ================================================================================ */



#ifdef __cplusplus
}
#endif

#endif /* MEMCC_STACK_H */