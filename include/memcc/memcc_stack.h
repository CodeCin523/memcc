#ifndef MEMCC_STACK_H
#define MEMCC_STACK_H

#ifdef __cplusplus
extern "C" {
#endif


#include "memcc.h"


/* ================================================================================ */
/*  TYPEDEF                                                                         */
/* ================================================================================ */

#define MEMCC_DFSTACK_FLAGS (0x80000000u)
#define MEMCC_DFSTACK_VALUE (0x7FFFFFFFu)

struct memcc_dfmeta {
    uint32_t lcount; // distance in bytes to previous meta & flag collapse
    uint32_t ncount; // distance in bytes to next meta
};

typedef struct memcc_dfstack {
    uint8_t *pool; // start of memory
    uint8_t *last; // points to current top meta
    uint32_t size; // total pool size in bytes
} memcc_dfstack_t;


/* ================================================================================ */
/*  DEFERRED_STACK                                                                  */
/* ================================================================================ */

static inline void memcc_setup_dfstack(memcc_dfstack_t *dfstack, void *pool, size_t size) {
    MEMCC_CHECK(dfstack && pool && size > 0, /*void*/);

    dfstack->pool = (uint8_t *)memcc_align_forward(pool, 8);
    dfstack->size = memcc_clamp(size - (dfstack->pool - (uint8_t *)pool), 0, UINT32_MAX);
    dfstack->last = dfstack->pool;

    MEMCC_ZERO_ALLOC(dfstack->pool, dfstack->size);

    struct memcc_dfmeta *meta = (struct memcc_dfmeta *)dfstack->last;
    meta->lcount = 0;
    meta->ncount = 0;
}
static inline void memcc_teardown_dfstack(memcc_dfstack_t *dfstack) {
    MEMCC_CHECK(dfstack, /*void*/);

    dfstack->pool = NULL;
    dfstack->last = NULL;
    dfstack->size = 0;
}


#ifdef __cplusplus
}
#endif

#endif /* MEMCC_STACK_H */