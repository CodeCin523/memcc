#ifndef MEMCC_BLOCK_H
#define MEMCC_BLOCK_H

#ifdef __cplusplus
extern "C" {
#endif


#include "memcc.h"


/* ================================================================================ */
/*  TYPEDEF                                                                         */
/* ================================================================================ */

// self-bitmap block
typedef struct memcc_sbblock {
    uint8_t *pool;          // start of the pointer, after the bitmap
    uint32_t block_num;     // total cell count
    uint16_t bitmap_size;   // size of bitmap buffer
    uint16_t block_size;    // size of single cell
} memcc_sbblock_t;

// static-size block
typedef struct memcc_ssblock {

};
// vector-size block
typedef struct memcc_vsblock {

};


/* ================================================================================ */
/*  SELF_BITMAP_BLOCK                                                               */
/* ================================================================================ */

// TODO : MEMCC_ZERO_ALLLOC and MEMCC_ZERO_FREE

static inline void memcc_setup_sbblock_tu(memcc_sbblock_t *sbblock, void *pool, uint32_t pool_size, uint16_t block_size) {
    MEMCC_CHECK(sbblock && pool && memcc_pow2(block_size), /*void*/);

    uint8_t *base = (uint8_t *)pool;
    uint8_t *end  = base + pool_size;

    size_t alignment = block_size;

    // Align end down to avoid partial block overrun
    end = (uint8_t *)memcc_align_floor(end, alignment);

    // Initial estimate
    uint32_t cc = (uint32_t)((end - base) / block_size);

    uint32_t bbc = 0;
    uint8_t *aligned_pool = NULL;

    while (1) {
        bbc = (cc + 7) / 8;

        aligned_pool =
            (uint8_t *)memcc_align_ceil(base + bbc, alignment);

        // No room for blocks
        MEMCC_CHECK(aligned_pool >= end, /*void*/);
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
static inline void memcc_teardown_sbblock_tu(memcc_sbblock_t *sbblock) {
    MEMCC_CHECK(sbblock, /*void*/);

    // MEMCC_ZERO_FREE(sbblock->pool - sbblock->bitmap_size, sbblock->block_size + sbblock->block_num * sbblock->block_size);

    sbblock->pool = NULL;
    sbblock->block_num = 0;
    sbblock->bitmap_size = 0;
    sbblock->block_size = 0;
}

static inline uint32_t memcc_sbblock_alloc_ts(memcc_sbblock_t *sbblock) {
    MEMCC_CHECK(sbblock, /*void*/);

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

static inline uint32_t memcc_sbblock_find_run_ts(memcc_sbblock_t *sbblock, uint32_t count) {
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
static inline uint32_t memcc_sbblock_alloc_run_tu(memcc_sbblock_t *sbblock, uint32_t idx, uint32_t count) {
    MEMCC_CHECK(sbblock && idx + count <= sbblock->block_num, UINT32_MAX);

    uint8_t *bitmap = sbblock->pool - sbblock->bitmap_size;

    for(uint32_t i = idx; i < idx + count; ++i) {
        uint32_t idx_byte = i / 8;
        uint32_t idx_bit = i % 8;

        bitmap[idx_byte] |= (1 << idx_bit);
    }

    return idx;
}

static inline void memcc_sbblock_free_ts(memcc_sbblock_t *sbblock, uint32_t idx) {
    MEMCC_CHECK(sbblock && idx < sbblock->block_num, /*void*/);
    
    uint8_t *bitmap = sbblock->pool - sbblock->bitmap_size;
    uint32_t idx_byte = idx / 8;
    uint32_t idx_bit = idx % 8;

    bitmap[idx_byte] &= ~(1 << idx_bit); // mark block free
}
static inline void memcc_sbblock_free_run_tu(memcc_sbblock_t *sbblock, uint32_t idx, uint32_t count) {
    MEMCC_CHECK(sbblock && idx + count <= sbblock->block_num, UINT32_MAX);

    uint8_t *bitmap = sbblock->pool - sbblock->bitmap_size;

    for(uint32_t i = idx; i < idx + count; ++i) {
        uint32_t idx_byte = i / 8;
        uint32_t idx_bit = i % 8;

        bitmap[idx_byte] &= ~(1 << idx_bit);
    }
}

static inline void *memcc_sbblock_get_ts(memcc_sbblock_t *sbblock, uint32_t idx) {
    return sbblock->pool + idx * sbblock->block_size;
}


/* ================================================================================ */
/*  INTERFACE ALIAS                                                                 */
/* ================================================================================ */


#ifdef __cplusplus
}
#endif

#endif /* MEMCC_BLOCK_H */