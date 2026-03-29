#define MEMCC_STACK_IMPLEMENTATION
#include <memcc/memcc_stack.h>
#include <stdio.h>
#include <stdint.h>

/* ================================================================================ */
/*  HISTORY TRACKING                                                                */
/* ================================================================================ */

struct entry {
    struct memcc_dfmeta *head;
    uint32_t asked_size;
};

#define HEAD_HISTORY_SIZE 256

static uint32_t head_history_count = 0;
static struct entry head_history[HEAD_HISTORY_SIZE] = {0};


static void print_history(void) {
    printf("Head History (%u)\n", head_history_count);

    for (uint32_t i = 0; i < head_history_count; ++i) {
        struct memcc_dfmeta *head = head_history[i].head;

        uint32_t head_lc = head->lcount & MEMCC_DFSTACK_VALUE;
        uint32_t head_nc = head->ncount & MEMCC_DFSTACK_VALUE;

        printf("  [%03u] size:%u\n", i, head_history[i].asked_size);
        printf("         head:%p (lc:%u nc:%u)\n",
               (void*)head, head_lc, head_nc);

        if (head_nc == 0) {
            printf("         foot: <none> (ncount == 0)\n");
        } else {
            struct memcc_dfmeta *foot = head + head_nc;

            uint32_t foot_lc = foot->lcount & MEMCC_DFSTACK_VALUE;
            uint32_t foot_nc = foot->ncount & MEMCC_DFSTACK_VALUE;
            uint32_t foot_flag = (foot->lcount & MEMCC_DFSTACK_FLAGS) ? 1 : 0;

            if (foot == head) {
                printf("         foot: <same as head>\n");
            } else {
                printf("         foot:%p (lc:%u nc:%u flags:%u)\n",
                       (void*)foot, foot_lc, foot_nc, foot_flag);
            }
        }
    }
}

static void push_history(memcc_dfstack_t *dfstack, uint32_t asked_size) {
    struct memcc_dfmeta *foot = (struct memcc_dfmeta *)dfstack->last;
    struct memcc_dfmeta *head = foot - (foot->lcount & MEMCC_DFSTACK_VALUE);

    if (head_history_count >= HEAD_HISTORY_SIZE) {
        printf("History overflow\n");
        return;
    }

    head_history[head_history_count].head = head;
    head_history[head_history_count].asked_size = asked_size;
    head_history_count++;

    print_history();
}

static void clear_history(void) {
    for (uint32_t i = 0; i < head_history_count; ++i) {
        head_history[i].head = NULL;
        head_history[i].asked_size = 0;
    }

    head_history_count = 0;
}

/* ========================================================================== */
/*  VALIDATION                                                                */
/* ========================================================================== */

static void validate_links(void) {
    printf("Validating links...\n");

    for (uint32_t i = 0; i < head_history_count; ++i) {
        struct memcc_dfmeta *h = head_history[i].head;
        uint32_t n = h->ncount & MEMCC_DFSTACK_VALUE;

        // 🔑 Skip collapsed / inactive nodes
        if (n == 0)
            continue;

        struct memcc_dfmeta *f = h + n;

        if ((f->lcount & MEMCC_DFSTACK_VALUE) != n) {
            printf("  BROKEN: head %p -> foot %p mismatch (lc:%u nc:%u)\n",
                (void*)h,
                (void*)f,
                f->lcount & MEMCC_DFSTACK_VALUE,
                n
            );
        }
    }
}

/* ========================================================================== */
/*  TEST SCENARIOS                                                            */
/* ========================================================================== */

static void test_basic(memcc_dfstack_t *stack) {
    printf("\n==== BASIC PUSH/POP ====\n");

    void *a = memcc_dfstack_push(stack, 32);
    push_history(stack, 32);

    void *b = memcc_dfstack_push(stack, 64);
    push_history(stack, 64);

    memcc_dfstack_pop(stack);
    printf("After pop b\n");
    print_history();

    memcc_dfstack_pop(stack);
    printf("After pop a\n");
    print_history();

    (void)a; (void)b;
}

/* -------------------------------------------------------------------------- */

static void test_deferred(memcc_dfstack_t *stack) {
    printf("\n==== DEFERRED FREE ====\n");

    void *a = memcc_dfstack_push(stack, 32);
    push_history(stack, 32);

    void *b = memcc_dfstack_push(stack, 64);
    push_history(stack, 64);

    void *c = memcc_dfstack_push(stack, 48);
    push_history(stack, 48);

    printf("\n-- free middle (b) --\n");
    memcc_dfstack_pop_addr(stack, b);
    print_history();
    validate_links();

    printf("\n-- free top (c) --\n");
    memcc_dfstack_pop(stack);
    print_history();
    validate_links();

    printf("\n-- free a (should collapse all) --\n");
    memcc_dfstack_pop_addr(stack, a);
    print_history();
    validate_links();
}

/* -------------------------------------------------------------------------- */

static void test_interleaved(memcc_dfstack_t *stack) {
    printf("\n==== INTERLEAVED ====\n");

    void *a = memcc_dfstack_push(stack, 16);
    push_history(stack, 16);

    void *b = memcc_dfstack_push(stack, 16);
    push_history(stack, 16);

    void *c = memcc_dfstack_push(stack, 16);
    push_history(stack, 16);

    void *d = memcc_dfstack_push(stack, 16);
    push_history(stack, 16);

    printf("\n-- free b, then d --\n");
    memcc_dfstack_pop_addr(stack, b);
    memcc_dfstack_pop_addr(stack, d);
    print_history();
    validate_links();

    printf("\n-- free c (should collapse b+d) --\n");
    memcc_dfstack_pop_addr(stack, c);
    print_history();
    validate_links();

    printf("\n-- free a (final collapse) --\n");
    memcc_dfstack_pop_addr(stack, a);
    print_history();
    validate_links();
}

/* -------------------------------------------------------------------------- */

static void test_restore(memcc_dfstack_t *stack) {
    printf("\n==== RESTORE MARK ====\n");

    void *a = memcc_dfstack_push(stack, 16);
    push_history(stack, 16);

    void *b = memcc_dfstack_push(stack, 32);
    push_history(stack, 32);

    void *c = memcc_dfstack_push(stack, 48);
    push_history(stack, 48);

    // mark after b
    void *mark = memcc_dfstack_mark(stack);
    printf("-- mark taken after b\n");

    void *d = memcc_dfstack_push(stack, 64);
    push_history(stack, 64);

    void *e = memcc_dfstack_push(stack, 80);
    push_history(stack, 80);

    printf("\n-- restore to mark (should remove d + e)\n");
    memcc_dfstack_restore(stack, mark);
    print_history();
    validate_links();

    printf("\n-- pop remaining c, b, a --\n");
    memcc_dfstack_pop(stack);
    memcc_dfstack_pop(stack);
    memcc_dfstack_pop(stack);
    print_history();
    validate_links();
}

/* -------------------------------------------------------------------------- */

static void test_stress(memcc_dfstack_t *stack) {
    printf("\n==== STRESS ====\n");

    void *ptrs[32] = {0};

    for (int i = 0; i < 16; ++i) {
        ptrs[i] = memcc_dfstack_push(stack, (i + 1) * 8);
        push_history(stack, (i + 1) * 8);
    }

    printf("\n-- free every other --\n");
    for (int i = 0; i < 16; i += 2) {
        memcc_dfstack_pop_addr(stack, ptrs[i]);
    }
    print_history();
    validate_links();

    printf("\n-- pop remaining from top --\n");
    for (int i = 15; i >= 0; --i) {
        memcc_dfstack_pop(stack);
    }

    print_history();
    validate_links();
}

/* ================================================================================ */
/*  MAIN                                                                            */
/* ================================================================================ */

static void reset_all(memcc_dfstack_t *stack) {
    memcc_dfstack_clear(stack);
    clear_history();
}

int main(void) {
    uint8_t buffer[4096];

    memcc_dfstack_t stack;
    memcc_setup_dfstack(&stack, buffer, sizeof(buffer));
    printf("buffer : %p\npool : %p\n", buffer, stack.pool);

    /*
    struct memcc_dfmeta *meta_01 = (struct memcc_dfmeta *) stack.last;
    printf("meta_00         : %p, lc : %d, nc : %d\n", meta_01, meta_01->lcount, meta_01->ncount);
    printf("\n");

    memcc_dfstack_push(&stack, 32);
    printf("meta_00         : %p, lc : %d, nc : %d\n", meta_01, meta_01->lcount, meta_01->ncount);
    struct memcc_dfmeta *meta_02 = (struct memcc_dfmeta *) stack.last;
    struct memcc_dfmeta *meta_03 = meta_02 - meta_02->lcount;
    printf("meta_00_moved   : %p, lc : %d, nc : %d\n", meta_03, meta_03->lcount, meta_03->ncount);
    printf("meta_01         : %p, lc : %d, nc : %d\n", meta_02, meta_02->lcount, meta_02->ncount);
    printf("\n");

    memcc_dfstack_push(&stack, 64);
    struct memcc_dfmeta *meta_04 = (struct memcc_dfmeta *) stack.last;
    struct memcc_dfmeta *meta_05 = meta_04 - meta_04->lcount;
    struct memcc_dfmeta *meta_01_pair = meta_05 - meta_05->lcount;
    printf("meta_00         : %p, lc : %d, nc : %d\n", meta_01, meta_01->lcount, meta_01->ncount);
    printf("meta_00_moved   : %p, lc : %d, nc : %d\n", meta_03, meta_03->lcount, meta_03->ncount);
    printf("meta_00_recalc  : %p, lc : %d, nc : %d\n", meta_01_pair, meta_01_pair->lcount, meta_01_pair->ncount);
    printf("meta_01         : %p, lc : %d, nc : %d\n", meta_02, meta_02->lcount, meta_02->ncount);
    printf("meta_01_moved   : %p, lc : %d, nc : %d\n", meta_05, meta_05->lcount, meta_05->ncount);
    printf("meta_02         : %p, lc : %d, nc : %d\n", meta_04, meta_04->lcount, meta_04->ncount);
    printf("\n");
    */

    test_basic(&stack);
    reset_all(&stack);

    test_deferred(&stack);
    reset_all(&stack);

    test_interleaved(&stack);
    reset_all(&stack);

    test_restore(&stack);
    reset_all(&stack);
    
    memcc_teardown_dfstack(&stack);
    return 0;
}