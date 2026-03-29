/* ================================================================================ */
/*  DEFINITION                                                                      */
/* ================================================================================ */
#ifndef MEMCC_BLOCK_H
#define MEMCC_BLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "memcc.h"


/* ================================================================================ */
/*  SELF_BITMAP_BLOCK                                                               */
/* ================================================================================ */

/*invariants (sbblock)
 * └─ TODO
 */

typedef struct memcc_sbblock {
    uint8_t *pool;          // start of the pointer, after the bitmap
    uint32_t block_num;     // total cell count
    uint16_t bitmap_size;   // size of bitmap buffer
    uint16_t block_size;    // size of single cell
} memcc_sbblock_t;

void memcc_setup_sbblock_tu         (memcc_sbblock_t *sbblock, void *pool, size_t pool_size, size_t block_size, size_t block_align);
void memcc_teardown_sbblock_tu      (memcc_sbblock_t *sbblock);

uint32_t memcc_sbblock_alloc_ts     (memcc_sbblock_t *sbblock);
uint32_t memcc_sbblock_find_run_ts  (memcc_sbblock_t *sbblock, uint32_t count);
uint32_t memcc_sbblock_alloc_run_tu (memcc_sbblock_t *sbblock, uint32_t idx, uint32_t count);
void memcc_sbblock_free_ts          (memcc_sbblock_t *sbblock, uint32_t idx);
void memcc_sbblock_free_run_tu      (memcc_sbblock_t *sbblock, uint32_t idx, uint32_t count);

static inline void *memcc_sbblock_get_ts(memcc_sbblock_t *sbblock, uint32_t idx) {
    return sbblock->pool + idx * sbblock->block_size;
}


/* ================================================================================ */
/*  STATIC_SIZE_BLOCK                                                               */
/* ================================================================================ */

/*invariants (ssblock)
 * └─ TODO
 */

typedef struct memcc_ssblock {

};


/* ================================================================================ */
/*  VECTOR_SIZE_BLOCK                                                               */
/* ================================================================================ */

/*invariants (vsblock)
 * └─ TODO
 */

typedef struct memcc_vsblock {

};


/* ================================================================================ */
/*  ALIAS                                                                           */
/* ================================================================================ */

// SELF_BITMAP_BLOCK
#define memcc_setup_sbblock(...)        memcc_setup_sbblock_tu(__VA_ARGS__,  alignof(max_align_t))
#define memcc_setup_sbblock_align(...)  memcc_setup_sbblock_tu(__VA_ARGS__)
#define memcc_setup_sbblock_type(block, pool, pool_size, type) \
                                        memcc_setup_sbblock_tu(block, pool, pool_size, sizeof(type), alignof(type))
#define memcc_teardown_sbblock(...)     memcc_teardown_sbblock_tu(__VA_ARGS__)

#define memcc_sbblock_alloc(...)        memcc_sbblock_alloc_ts(__VA_ARGS__)
#define memcc_sbblock_find_run(...)     memcc_sbblock_find_run_ts(__VA_ARGS__)
#define memcc_sbblock_alloc_run(...)    memcc_sbblock_alloc_run_tu(__VA_ARGS__)

#define memcc_sbblock_free(...)         memcc_sbblock_free_ts(__VA_ARGS__)
#define memcc_sbblock_free_run(...)     memcc_sbblock_free_run_tu(__VA_ARGS__)
#define memcc_sbblock_get(...)          memcc_sbblock_get_ts(__VA_ARGS__)

// STATIC_SIZE_BLOCK

// VECTOR_SIZE_BLOCK


#ifdef __cplusplus
}
#endif

#endif /* MEMCC_BLOCK_H */


/* ================================================================================ */
/*  IMPLEMENTATION                                                                  */
/* ================================================================================ */
#define MEMCC_BLOCK_IMPLEMENTATION
#ifdef MEMCC_BLOCK_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif


/* ================================================================================ */
/*  SELF_BITMAP_BLOCK                                                               */
/* ================================================================================ */

void memcc_setup_sbblock_tu(memcc_sbblock_t *sbblock, void *pool, size_t pool_size, size_t block_size, size_t block_align) {
    MEMCC_CHECK(sbblock && pool && memcc_pow2(block_size), /*void*/);

    uint8_t *base = (uint8_t *)pool;
    uint8_t *end  = base + pool_size;

    // Align end down to avoid partial block overrun
    end = (uint8_t *)memcc_align_floor(end, block_align);

    // Initial estimate
    uint32_t cc = (uint32_t)((end - base) / block_size);

    uint32_t bbc = 0;
    uint8_t *aligned_pool = NULL;

    while (1) {
        bbc = (cc + 7) / 8;

        aligned_pool =
            (uint8_t *)memcc_align_ceil(base + bbc, block_align);
        // No room for blocks
        MEMCC_CHECK(aligned_pool < end, /*void*/);
        // if (aligned_pool >= end) {
        //     sbblock->pool = NULL;
        //     sbblock->block_num = 0;
        //     sbblock->bitmap_size = 0;
        //     sbblock->block_size = block_size;
        //     return;
        // }

        size_t usable = (size_t)(end - aligned_pool);
        uint32_t new_cc = (uint32_t)(usable / block_size);

        // Converged
        if (new_cc == cc) {
            break;
        }

        cc = new_cc;
    }

    // Finalize
    sbblock->pool        = aligned_pool;
    sbblock->block_num   = cc;
    sbblock->bitmap_size = bbc;
    sbblock->block_size  = block_size;

    // Bitmap at start of buffer
    memcc_zero(base, bbc);
}
void memcc_teardown_sbblock_tu(memcc_sbblock_t *sbblock) {
    MEMCC_CHECK(sbblock, /*void*/);

    // MEMCC_ZERO_FREE(sbblock->pool - sbblock->bitmap_size, sbblock->block_size + sbblock->block_num * sbblock->block_size);

    sbblock->pool = NULL;
    sbblock->block_num = 0;
    sbblock->bitmap_size = 0;
    sbblock->block_size = 0;
}

uint32_t memcc_sbblock_alloc_ts(memcc_sbblock_t *sbblock) {
    MEMCC_CHECK(sbblock, UINT32_MAX);

    uint8_t *bitmap = sbblock->pool - sbblock->bitmap_size;

    for(uint32_t i = 0; i < sbblock->bitmap_size; ++i) {
        if(bitmap[i] == 0xFF) continue;

        // Can be replace by a single __buildtin
        for(uint32_t j = 0; j < 8; ++j) {
            uint8_t jmask = 1 << j;
            if ((bitmap[i] & jmask) == 0) {
                bitmap[i] |= jmask;
                return i * 8 + j;
            }
        }
    }

    return UINT32_MAX;
}

uint32_t memcc_sbblock_find_run_ts(memcc_sbblock_t *sbblock, uint32_t count) {
    MEMCC_CHECK(sbblock && count > 0, UINT32_MAX);
    uint32_t run_start = 0, run_len = 0;
    uint8_t *bitmap = sbblock->pool - sbblock->bitmap_size;
    
    for(uint32_t i = 0; i < sbblock->bitmap_size * 8; i++) {
        uint32_t idx_byte = i / 8;
        uint32_t idx_bit = i % 8;

        if((bitmap[idx_byte] >> idx_bit & 1) != 1) {
            if(run_len == 0) run_start = i;
            ++run_len;
            if(run_len == count) return run_start;
        } else {
            run_len = 0;
        }
    }

    return UINT32_MAX;
}
uint32_t memcc_sbblock_alloc_run_tu(memcc_sbblock_t *sbblock, uint32_t idx, uint32_t count) {
    MEMCC_CHECK(sbblock && idx + count <= sbblock->block_num, UINT32_MAX);

    uint8_t *bitmap = sbblock->pool - sbblock->bitmap_size;

    for(uint32_t i = idx; i < idx + count; ++i) {
        uint32_t idx_byte = i / 8;
        uint32_t idx_bit = i % 8;

        bitmap[idx_byte] |= (1 << idx_bit);
    }

    return idx;
}

void memcc_sbblock_free_ts(memcc_sbblock_t *sbblock, uint32_t idx) {
    MEMCC_CHECK(sbblock && idx < sbblock->block_num, /*void*/);
    
    uint8_t *bitmap = sbblock->pool - sbblock->bitmap_size;
    uint32_t idx_byte = idx / 8;
    uint32_t idx_bit = idx % 8;

    bitmap[idx_byte] &= ~(1 << idx_bit); // mark block free
}
void memcc_sbblock_free_run_tu(memcc_sbblock_t *sbblock, uint32_t idx, uint32_t count) {
    MEMCC_CHECK(sbblock && idx + count <= sbblock->block_num,  /*void*/);

    uint8_t *bitmap = sbblock->pool - sbblock->bitmap_size;

    for(uint32_t i = idx; i < idx + count; ++i) {
        uint32_t idx_byte = i / 8;
        uint32_t idx_bit = i % 8;

        bitmap[idx_byte] &= ~(1 << idx_bit);
    }
}

/* ================================================================================ */
/*  STATIC_SIZE_BLOCK                                                               */
/* ================================================================================ */


/* ================================================================================ */
/*  VECTOR_SIZE_BLOCK                                                               */
/* ================================================================================ */


#ifdef __cplusplus
}
#endif

#endif /* MEMCC_BLOCK_IMPLEMENTATION */
