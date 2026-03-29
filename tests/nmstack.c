#define MEMCC_STACK_IMPLEMENTATION
#include <memcc/memcc_stack.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* ========================================================================== */
/*  ASSERT                                                                     */
/* ========================================================================== */

#define ASSERT(cond, msg) \
    do { if (!(cond)) { \
        printf("ASSERT FAILED: %s\n", msg); \
        exit(1); \
    }} while(0)

/* ========================================================================== */
/*  HELPERS                                                                    */
/* ========================================================================== */

static void check_bounds(memcc_nmstack_t *s, void *p) {
    ASSERT(p >= (void*)s->pool, "ptr < pool");
    ASSERT(p < (void*)(s->pool + s->size), "ptr >= pool+size");
}

static void check_align(void *p, size_t align) {
    ASSERT(((uintptr_t)p % align) == 0, "bad alignment");
}

static void check_zero(uint8_t *p, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (p[i] != 0) {
            printf("NON ZERO at %zu\n", i);
            exit(1);
        }
    }
}

/* ========================================================================== */
/*  BASIC                                                                      */
/* ========================================================================== */

static void test_basic(memcc_nmstack_t *s) {
    printf("\n==== BASIC ====\n");

    void *a = memcc_nmstack_push(s, 32);
    ASSERT(a, "push a failed");

    void *b = memcc_nmstack_push(s, 64);
    ASSERT(b, "push b failed");

    ASSERT(b > a, "stack didn't grow");

    check_bounds(s, a);
    check_bounds(s, b);

    printf("BASIC OK\n");
}

/* ========================================================================== */
/*  ALIGNMENT                                                                  */
/* ========================================================================== */

static void test_alignment(memcc_nmstack_t *s) {
    printf("\n==== ALIGNMENT ====\n");

    void *a = memcc_nmstack_push_align(s, 8, 8);
    void *b = memcc_nmstack_push_align(s, 8, 16);
    void *c = memcc_nmstack_push_align(s, 8, 32);

    check_align(a, 8);
    check_align(b, 16);
    check_align(c, 32);

    printf("ALIGNMENT OK\n");
}

/* ========================================================================== */
/*  ZERO ON ALLOC                                                              */
/* ========================================================================== */

static void test_zero_alloc(memcc_nmstack_t *s) {
    printf("\n==== ZERO ALLOC ====\n");

    uint8_t *a = memcc_nmstack_push(s, 64);
    ASSERT(a, "push failed");

    check_zero(a, 64);

    printf("ZERO ALLOC OK\n");
}

/* ========================================================================== */
/*  MARK / RESTORE                                                             */
/* ========================================================================== */

static void test_mark_restore(memcc_nmstack_t *s) {
    printf("\n==== MARK / RESTORE ====\n");

    uint8_t *a = memcc_nmstack_push(s, 32);
    ASSERT(a, "push a failed");

    void *mark = memcc_nmstack_mark(s);

    uint8_t *b = memcc_nmstack_push(s, 64);
    ASSERT(b, "push b failed");

    // dirty memory
    for (int i = 0; i < 64; ++i)
        b[i] = 0xAA;

    memcc_nmstack_restore(s, mark);

    // restored region should be zeroed
    uint8_t *b2 = memcc_nmstack_push(s, 64);
    check_zero(b2, 64);

    printf("MARK/RESTORE OK\n");
}

/* ========================================================================== */
/*  BOUNDS                                                                     */
/* ========================================================================== */

static void test_bounds(memcc_nmstack_t *s) {
    printf("\n==== BOUNDS ====\n");

    size_t total = 0;

    while (1) {
        void *p = memcc_nmstack_push(s, 32);
        if (!p) break;
        total += 32;
    }

    printf("Filled: %zu bytes\n", total);

    void *fail = memcc_nmstack_push(s, 16);
    ASSERT(fail == NULL, "should fail");

    printf("BOUNDS OK\n");
}

/* ========================================================================== */
/*  CLEAR                                                                      */
/* ========================================================================== */

static void test_clear(memcc_nmstack_t *s) {
    printf("\n==== CLEAR ====\n");

    void *a = memcc_nmstack_push(s, 32);
    ASSERT(a, "push failed");

    memcc_nmstack_clear(s);

    void *b = memcc_nmstack_push(s, 32);
    ASSERT(b == a, "clear didn't reset");

    printf("CLEAR OK\n");
}

/* ========================================================================== */
/*  STRESS                                                                     */
/* ========================================================================== */

static void test_stress(memcc_nmstack_t *s) {
    printf("\n==== STRESS ====\n");

    void *marks[64];
    int mark_count = 0;

    for (int i = 0; i < 10000; ++i) {
        int r = rand() % 3;

        if (r == 0) {
            void *p = memcc_nmstack_push(s, rand() % 128 + 1);
            if (p) check_bounds(s, p);
        }
        else if (r == 1 && mark_count < 64) {
            marks[mark_count++] = memcc_nmstack_mark(s);
        }
        else if (r == 2 && mark_count > 0) {
            int idx = rand() % mark_count;
            memcc_nmstack_restore(s, marks[idx]);
            mark_count = idx;
        }
    }

    printf("STRESS OK\n");
}

/* ========================================================================== */
/*  MAIN                                                                       */
/* ========================================================================== */

int main(void) {
    uint8_t buffer[4096];

    memcc_nmstack_t stack;
    memcc_setup_nmstack(&stack, buffer, sizeof(buffer));

    test_basic(&stack);
    memcc_nmstack_clear(&stack);

    test_alignment(&stack);
    memcc_nmstack_clear(&stack);

    test_zero_alloc(&stack);
    memcc_nmstack_clear(&stack);

    test_mark_restore(&stack);
    memcc_nmstack_clear(&stack);

    test_bounds(&stack);
    memcc_nmstack_clear(&stack);

    test_clear(&stack);
    memcc_nmstack_clear(&stack);

    test_stress(&stack);

    memcc_teardown_nmstack(&stack);

    printf("\nALL TESTS PASSED\n");
    return 0;
}