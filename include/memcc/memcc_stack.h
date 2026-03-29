/* ================================================================================ */
/*  DEFINITION                                                                      */
/* ================================================================================ */
#ifndef MEMCC_STACK_H
#define MEMCC_STACK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "memcc.h"


/* ================================================================================ */
/*  DEFERRED_STACK                                                                  */
/* ================================================================================ */

/*invariants (dfstack)
 * ├─ structure
 * |  ├─ each allocation has:
 * |  |  └─ [head meta] ... [foot meta]
 * |  ├─ head->ncount = distance to foot (in meta units)
 * |  └─ foot->lcount = distance to head (+ optional FLAG)
 * ├─ core relation
 * |  ├─ head + head->ncount == foot
 * |  ├─ foot - (foot->lcount & VALUE) == head
 * |  ├─ head->ncount == (foot->lcount & VALUE)
 * |  ├─ head->ncount > 0
 * |  └─ head < foot
 * ├─ bounds
 * |  └─ pool <= head < foot < pool + size
 * ├─ last pointer
 * |  └─ last points to current top foot
 * ├─ navigation
 * |  ├─ forward  : foot = head + head->ncount
 * |  └─ backward : head = foot - (foot->lcount & VALUE)
 * ├─ deferred free
 * |  ├─ (foot->lcount & FLAG) != 0 → block is logically freed
 * |  └─ structure remains valid until collapse
 * ├─ collapse
 * |  ├─ occurs when flagged block reaches top
 * |  └─ effect:
 * |     ├─ head->ncount = 0
 * |     ├─ foot->lcount = 0
 * |     └─ foot->ncount = 0
 * ├─ push
 * |  ├─ head->ncount = span
 * |  ├─ foot->lcount = span
 * |  ├─ foot->ncount = 0
 * |  ├─ payload is properly aligned
 * |  ├─ head / foot aligned to alignof(meta)
 * |  └─ if head > previous head:
 * |     └─ chain must be repaired to remain navigable
 * ├─ pop
 * |  ├─ top pop:
 * |  |  └─ last = head
 * |  └─ deferred pop:
 * |     └─ foot->lcount |= FLAG
 * ├─ restore
 * |  ├─ valid mark : pool <= mark <= pool + size
 * |  ├─ last = resolved head
 * |  └─ cascade collapse still applies
 * └─ zeroing
 *    ├─ only unreachable memory may be zeroed
 *    ├─ must NOT zero:
 *    |  ├─ active head
 *    |  ├─ active foot
 *    |  └─ any reachable meta
 *    └─ zeroed region must be outside active structure
 */

struct memcc_dfmeta {
    uint32_t lcount; // distance in sizeof(memcc_dfmeta) to previous meta & flag collapse
    uint32_t ncount; // distance in sizeof(memcc_dfmeta) to next meta
};

typedef struct memcc_dfstack {
    uint8_t *pool;  // start of memory
    uint8_t *last;  // points to current top meta
    size_t   size;  // total byte count in pool
} memcc_dfstack_t;

void memcc_setup_dfstack_tu     (memcc_dfstack_t *dfstack, void *pool, size_t size);
void memcc_teardown_dfstack_tu  (memcc_dfstack_t *dfstack);

void *memcc_dfstack_push_tu     (memcc_dfstack_t *dfstack, size_t size, size_t align);
void memcc_dfstack_pop_tu       (memcc_dfstack_t *dfstack, void *addr);
void *memcc_dfstack_mark_tu     (memcc_dfstack_t *dfstack);
void memcc_dfstack_restore_tu   (memcc_dfstack_t *dfstack, void *mark);
void memcc_dfstack_clear_tu     (memcc_dfstack_t *dfstack);


/* ================================================================================ */
/*  NO_META_STACK                                                                   */
/* ================================================================================ */

/*invariants (nmstack)
 * ├─ global
 * |  ├─ pool <= top <= pool + size
 * |  ├─ top moves forward on push
 * |  └─ top moves backward only on restore / clear
 * ├─ allocation
 * |  ├─ payload = align_ceil(previous_top, align)
 * |  ├─ top = payload + size
 * |  ├─ payload >= previous_top
 * |  └─ payload + size <= pool + size
 * ├─ ordering
 * |  └─ allocations are strictly non-overlapping and ordered
 * ├─ alignment
 * |  └─ payload % align == 0
 * ├─ padding
 * |  └─ alignment padding is consumed (not reused until restore / clear)
 * ├─ mark / restore
 * |  ├─ valid mark : pool <= mark <= pool + size
 * |  ├─ restore    : top = mark
 * |  └─ zero       : [mark, old_top) is zeroed
 * └─ clear
 *    ├─ top = pool
 *    └─ entire pool is zeroed
 */

typedef struct memcc_nmstack {
    uint8_t *pool;
    uint8_t *top;
    size_t   size;
} memcc_nmstack_t;

void memcc_setup_nmstack_tu     (memcc_nmstack_t *nmstack, void *pool, size_t size);
void memcc_teardown_nmstack_tu  (memcc_nmstack_t *nmstack);

void *memcc_nmstack_push_tu     (memcc_nmstack_t *nmstack, size_t size, size_t align);
void *memcc_nmstack_mark_tu     (memcc_nmstack_t *nmstack);
void memcc_nmstack_restore_tu   (memcc_nmstack_t *nmstack, void *mark);
void memcc_nmstack_clear_tu     (memcc_nmstack_t *nmstack);


/* ================================================================================ */
/*  ALIAS                                                                           */
/* ================================================================================ */

// DEFERRED_STACK
#define memcc_setup_dfstack(...)        memcc_setup_dfstack_tu(__VA_ARGS__)
#define memcc_teardown_dfstack(...)     memcc_teardown_dfstack_tu(__VA_ARGS__)

#define memcc_dfstack_push(...)         memcc_dfstack_push_tu(__VA_ARGS__, alignof(max_align_t))
#define memcc_dfstack_push_align(...)   memcc_dfstack_push_tu(__VA_ARGS__)
#define memcc_dfstack_push_type(stack,count,type) \
                                        memcc_dfstack_push_tu(stack, sizeof(type)*(count), alignof(type))
                                       
#define memcc_dfstack_pop(...)          memcc_dfstack_pop_tu(__VA_ARGS__, NULL)
#define memcc_dfstack_pop_addr(...)     memcc_dfstack_pop_tu(__VA_ARGS__)
#define memcc_dfstack_mark(...)         memcc_dfstack_mark_tu(__VA_ARGS__)
#define memcc_dfstack_restore(...)      memcc_dfstack_restore_tu(__VA_ARGS__)
#define memcc_dfstack_clear(...)        memcc_dfstack_clear_tu(__VA_ARGS__)

// NO_META_STACK
#define memcc_setup_nmstack(...)        memcc_setup_nmstack_tu(__VA_ARGS__)
#define memcc_teardown_nmstack(...)     memcc_teardown_nmstack_tu(__VA_ARGS__)

#define memcc_nmstack_push(...)         memcc_nmstack_push_tu(__VA_ARGS__, alignof(max_align_t))
#define memcc_nmstack_push_align(...)   memcc_nmstack_push_tu(__VA_ARGS__)
#define memcc_nmstack_push_type(stack,count,type) \
                                        memcc_nmstack_push_tu(stack, sizeof(type)*(count), alignof(type))

#define memcc_nmstack_mark(...)         memcc_nmstack_mark_tu(__VA_ARGS__)
#define memcc_nmstack_restore(...)      memcc_nmstack_restore_tu(__VA_ARGS__)
#define memcc_nmstack_clear(...)        memcc_nmstack_clear_tu(__VA_ARGS__)


#ifdef __cplusplus
}
#endif

#endif /* MEMCC_STACK_H */


/* ================================================================================ */
/*  IMPLEMENTATION                                                                  */
/* ================================================================================ */
#ifdef MEMCC_STACK_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif


/* ================================================================================ */
/*  DEFERRED_STACK                                                                  */
/* ================================================================================ */

#define MEMCC_DFSTACK_FLAGS (0x80000000u)
#define MEMCC_DFSTACK_VALUE (0x7FFFFFFFu)

void memcc_setup_dfstack_tu(memcc_dfstack_t *dfstack, void *pool, size_t size) {
    MEMCC_CHECK(dfstack && pool && size > 0, /*void*/);

    dfstack->pool = (uint8_t *) memcc_align_ceil(pool, alignof(max_align_t));
    dfstack->last = dfstack->pool;

    // get size, ajust with moved pool
    uintptr_t shift = (uintptr_t)dfstack->pool - (uintptr_t)pool;
    MEMCC_CHECK(size > shift, /*void*/);
    dfstack->size = size - shift;

    MEMCC_ZERO_ALLOC(dfstack->pool, dfstack->size);

#ifndef MEMCC_ZERO_ON_ALLOC
    struct memcc_dfmeta *meta = (struct memcc_dfmeta *)dfstack->last;
    meta->lcount = 0;
    meta->ncount = 0;
#endif
}
void memcc_teardown_dfstack_tu(memcc_dfstack_t *dfstack) {
    MEMCC_CHECK(dfstack, /*void*/);

    // MEMCC_ZERO_FREE(dfstack->pool, dfstack->size);

    dfstack->pool = NULL;
    dfstack->last = NULL;
    dfstack->size = 0;
}

void *memcc_dfstack_push_tu(memcc_dfstack_t *dfstack, size_t size, size_t align) {
    MEMCC_CHECK(dfstack && memcc_pow2(align), NULL);

    // calculate payload, span and next
    uint8_t *payload = (uint8_t *) memcc_align_ceil(
        dfstack->last + sizeof(struct memcc_dfmeta),
        align
    );

    // where is next
    struct memcc_dfmeta *foot = (struct memcc_dfmeta *) memcc_align_ceil(payload + size, alignof(struct memcc_dfmeta));
    // where are lasts
    struct memcc_dfmeta *last_head = (struct memcc_dfmeta *) dfstack->last;
    struct memcc_dfmeta *head = (struct memcc_dfmeta *) memcc_align_floor(
        payload - sizeof(struct memcc_dfmeta),
        alignof(struct memcc_dfmeta)
    );

    // Bounds check
    if((uint8_t *)(foot + 1) - dfstack->pool > dfstack->size)
        return NULL;

    MEMCC_CHECK(head >= last_head, NULL);
    if(head > last_head) {
        // tell the pair that we have moved.
        if(last_head->lcount != 0) {
            struct memcc_dfmeta *last_pair = last_head - (last_head->lcount & MEMCC_DFSTACK_VALUE);
            size_t back_span = (size_t)(head - last_pair);
            MEMCC_CHECK(back_span <= MEMCC_DFSTACK_VALUE, NULL);

            last_pair->ncount = (uint32_t)back_span;
            head->lcount = (uint32_t)back_span;

            // Quick fix to make restore GREAT AGAIN
            last_head->lcount = 0;
            last_head->ncount = head - last_head;
        } else { // make sure the meta at pool is always valid, re-implemented for cascade
            last_head->ncount = head - last_head;
            head->lcount = last_head->ncount | MEMCC_DFSTACK_FLAGS;
        }
    }

    size_t span = (size_t)(foot - head);

    MEMCC_CHECK(span <= MEMCC_DFSTACK_VALUE, NULL);
    
    head->ncount = (uint32_t)span;
    foot->lcount = (uint32_t)span;
    foot->ncount = 0;

    MEMCC_ZERO_ALLOC(head+1, (head->ncount-1)*sizeof(struct memcc_dfmeta));

    dfstack->last = (uint8_t *)foot;
    return payload;
}
void memcc_dfstack_pop_tu(memcc_dfstack_t *dfstack, void *addr) {
    MEMCC_CHECK(dfstack && dfstack->last != dfstack->pool, /*void*/);

    // behavior normal pop, without addr
    struct memcc_dfmeta *foot = (struct memcc_dfmeta *)dfstack->last;
    struct memcc_dfmeta *head = foot - (foot->lcount & MEMCC_DFSTACK_VALUE);

    // behavior with addr
    if(addr) {
        head = (struct memcc_dfmeta *) memcc_align_floor(
            (uint8_t *)addr - sizeof(struct memcc_dfmeta),
            alignof(struct memcc_dfmeta)
        );
        MEMCC_CHECK(head->ncount > 0, /*void*/);

        struct memcc_dfmeta *nfoot = head + head->ncount;

        MEMCC_CHECK((uint8_t *)nfoot >= dfstack->pool &&
                (uint8_t *)nfoot < dfstack->pool + dfstack->size, /*void*/);

        MEMCC_CHECK((nfoot->lcount & MEMCC_DFSTACK_VALUE) == head->ncount, /*void*/);

        // deffered free
        if(nfoot != foot) {
            nfoot->lcount |= MEMCC_DFSTACK_FLAGS;
            return;
        }

        foot = nfoot;
    }

    // normal behavior otherwise
    head->ncount = 0;
    foot->lcount = 0;
    foot->ncount = 0;
    dfstack->last = (uint8_t *)head;


    // cascade deffered collapses
    while(dfstack->last != dfstack->pool) {
        struct memcc_dfmeta *prev_foot = (struct memcc_dfmeta *)dfstack->last;

        if((prev_foot->lcount & MEMCC_DFSTACK_FLAGS) == 0)
            break;

        struct memcc_dfmeta *prev_head = prev_foot - (prev_foot->lcount & MEMCC_DFSTACK_VALUE);
        prev_head->ncount = 0;
        prev_foot->lcount = 0;
        prev_foot->ncount = 0;
        dfstack->last = (uint8_t *) prev_head;
    }

    MEMCC_ZERO_FREE(
        dfstack->last+1,
        (uint8_t *)head - dfstack->last
    );
}

void *memcc_dfstack_mark_tu(memcc_dfstack_t *dfstack) {
    MEMCC_CHECK(dfstack, NULL);
    return dfstack->last + sizeof(struct memcc_dfmeta);
}
void memcc_dfstack_restore_tu(memcc_dfstack_t *dfstack, void *mark) {
    uint8_t *mark8 = (uint8_t *)mark;

    MEMCC_CHECK(dfstack && mark8 >= dfstack->pool && mark8 <= (dfstack->pool + dfstack->size), /*void*/);

    struct memcc_dfmeta *head = (struct memcc_dfmeta *) memcc_align_floor(
            (uint8_t *)mark - sizeof(struct memcc_dfmeta),
            alignof(struct memcc_dfmeta)
        );

    if(head->lcount == 0 && mark8 != dfstack->pool) {
        head = (struct memcc_dfmeta *)(head + head->ncount);
    }
    head->ncount = 0;

    size_t len = dfstack->last - (uint8_t *)head - sizeof(struct memcc_dfmeta);
    dfstack->last = (uint8_t *)head;

    // cascade deffered collapses
    while(dfstack->last != dfstack->pool) {
        struct memcc_dfmeta *prev_foot = (struct memcc_dfmeta *)dfstack->last;

        if((prev_foot->lcount & MEMCC_DFSTACK_FLAGS) == 0)
            break;

        struct memcc_dfmeta *prev_head = prev_foot - (prev_foot->lcount & MEMCC_DFSTACK_VALUE);
        prev_head->ncount = 0;
        prev_foot->lcount = 0;
        prev_foot->ncount = 0;
        dfstack->last = (uint8_t *) prev_head;
    }

    MEMCC_ZERO_FREE(head + 1, len);
}

void memcc_dfstack_clear_tu(memcc_dfstack_t *dfstack) {
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
/*  NO_META_STACK                                                                   */
/* ================================================================================ */

void memcc_setup_nmstack_tu     (memcc_nmstack_t *nmstack, void *pool, size_t size) {
    MEMCC_CHECK(nmstack && pool && size > 0, /*void*/);

    nmstack->pool = (uint8_t *) memcc_align_ceil(pool, alignof(max_align_t));
    nmstack->top = nmstack->pool;

    // get size, ajust with moved pool
    uintptr_t shift = (uintptr_t)nmstack->pool - (uintptr_t)pool;
    MEMCC_CHECK(size > shift, /*void*/);
    nmstack->size = size - shift;

    MEMCC_ZERO_ALLOC(nmstack->pool, nmstack->size);
}
void memcc_teardown_nmstack_tu  (memcc_nmstack_t *nmstack) {
    MEMCC_CHECK(nmstack, /*void*/);

    nmstack->pool = NULL;
    nmstack->top = NULL;
    nmstack->size = 0;
}

void *memcc_nmstack_push_tu     (memcc_nmstack_t *nmstack, size_t size, size_t align) {
    MEMCC_CHECK(nmstack && memcc_pow2(align), NULL);

    uint8_t *payload = (uint8_t *) memcc_align_ceil(nmstack->top, align);
    uint8_t *ntop = (uint8_t *)payload + size;

    if (ntop > nmstack->pool + nmstack->size)
        return NULL;

    MEMCC_ZERO_ALLOC(payload, size);

    nmstack->top = ntop;
    return payload;
}

void *memcc_nmstack_mark_tu     (memcc_nmstack_t *nmstack) {
    MEMCC_CHECK(nmstack, NULL);
    return nmstack->top;
}
void memcc_nmstack_restore_tu   (memcc_nmstack_t *nmstack, void *mark) {
    uint8_t * mark8 = (uint8_t *) mark; 
    
    MEMCC_CHECK(nmstack && mark8 >= nmstack->pool && mark8 <= (nmstack->pool + nmstack->size), /*void*/);

    MEMCC_ZERO_FREE(mark8, nmstack->top - mark8);
    nmstack->top = mark8;
}

void memcc_nmstack_clear_tu     (memcc_nmstack_t *nmstack) {
    MEMCC_CHECK(nmstack, /*void*/);

    nmstack->top = nmstack->pool;
    MEMCC_ZERO_FREE(nmstack->pool, nmstack->size);
}


#ifdef __cplusplus
}
#endif

#endif /* MEMCC_STACK_IMPLEMENTATION */
