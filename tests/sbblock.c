#define MEMCC_BLOCK_IMPLEMENTATION
#include <memcc/memcc_block.h>
#include <stdio.h>
#include <string.h>

#define POOL_SIZE 256
#define BLOCK_SIZE 8


static void print_bitmap(memcc_sbblock_t *a)
{
    uint8_t *bitmap = a->pool - a->bitmap_size;

    printf("bitmap: ");

    for (uint32_t i = 0; i < a->bitmap_size; ++i) {
        for (uint32_t j = 0; j < 8; ++j) {
            printf("%u", (bitmap[i] >> j) & 1);
        }
        printf(" ");
    }

    printf("\n");
}


int main(void)
{
    printf("==== SBLOCK ALLOCATOR TEST ====\n\n");

    uint8_t pool[POOL_SIZE];
    memset(pool, 0, sizeof(pool));

    memcc_sbblock_t alloc;

    printf("setup allocator\n");
    memcc_setup_sbblock(&alloc, pool, POOL_SIZE, BLOCK_SIZE);
    print_bitmap(&alloc);
    printf("\n");


    /* ------------------------------------------ */
    printf("allocating 10 single blocks\n");

    uint32_t ids[32];

    for (int i = 0; i < 10; ++i) {
        ids[i] = memcc_sbblock_alloc(&alloc);
        printf("alloc -> %u\n", ids[i]);
        print_bitmap(&alloc);
    }

    printf("\n");


    /* ------------------------------------------ */
    printf("free some blocks to create holes\n");

    memcc_sbblock_free(&alloc, ids[2]);
    printf("free -> %u\n", ids[2]);
    print_bitmap(&alloc);

    memcc_sbblock_free(&alloc, ids[5]);
    printf("free -> %u\n", ids[5]);
    print_bitmap(&alloc);

    memcc_sbblock_free(&alloc, ids[7]);
    printf("free -> %u\n", ids[7]);
    print_bitmap(&alloc);

    printf("\n");


    /* ------------------------------------------ */
    printf("allocate 3 more singles (should reuse holes)\n");

    for (int i = 10; i < 13; ++i) {
        ids[i] = memcc_sbblock_alloc(&alloc);
        printf("alloc -> %u\n", ids[i]);
        print_bitmap(&alloc);
    }

    printf("\n");


    /* ------------------------------------------ */
    printf("find and allocate run of 4\n");

    uint32_t run = memcc_sbblock_find_run(&alloc, 4);
    printf("run start -> %u\n", run);

    if (run != UINT32_MAX) {
        memcc_sbblock_alloc_run(&alloc, run, 4);
    }

    print_bitmap(&alloc);
    printf("\n");


    /* ------------------------------------------ */
    printf("allocate 5 more singles\n");

    for (int i = 13; i < 18; ++i) {
        ids[i] = memcc_sbblock_alloc(&alloc);
        printf("alloc -> %u\n", ids[i]);
        print_bitmap(&alloc);
    }

    printf("\n");


    /* ------------------------------------------ */
    printf("free the run (count=4)\n");

    memcc_sbblock_free_run(&alloc, run, 4);
    print_bitmap(&alloc);
    printf("\n");


    /* ------------------------------------------ */
    printf("allocate another run of 3\n");

    uint32_t run2 = memcc_sbblock_find_run(&alloc, 3);
    printf("run start -> %u\n", run2);

    if (run2 != UINT32_MAX) {
        memcc_sbblock_alloc_run(&alloc, run2, 3);
    }

    print_bitmap(&alloc);
    printf("\n");


    /* ------------------------------------------ */
    printf("free a few single blocks\n");

    memcc_sbblock_free(&alloc, ids[0]);
    printf("free -> %u\n", ids[0]);
    print_bitmap(&alloc);

    memcc_sbblock_free(&alloc, ids[9]);
    printf("free -> %u\n", ids[9]);
    print_bitmap(&alloc);

    memcc_sbblock_free(&alloc, ids[12]);
    printf("free -> %u\n", ids[12]);
    print_bitmap(&alloc);

    printf("\n");


    /* ------------------------------------------ */
    printf("final allocations\n");

    for (int i = 18; i < 22; ++i) {
        ids[i] = memcc_sbblock_alloc(&alloc);
        printf("alloc -> %u\n", ids[i]);
        print_bitmap(&alloc);
    }

    printf("\n");


    printf("teardown allocator\n");
    memcc_teardown_sbblock(&alloc);

    printf("\n==== TEST COMPLETE ====\n");

    return 0;
}