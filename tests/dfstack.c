#include <memcc/memcc_stack.h>
#include <stdio.h>

struct entry {
    struct memcc_dfmeta *head;
    uint32_t asked_size;
};

#define                 head_history_size 256
uint32_t                head_history_count = 0;
struct entry           *head_history[head_history_size] = {};


static void print_history() {
    printf("Head History %d\n", head_history_count);
    for(uint32_t i = 0; i < head_history_count; ++i) {
        struct entry *entry = head_history[i];
        struct memcc_dfmeta *head = entry->head;
        printf("  {ptr:%p, datasize:%d, lc:%d, nc:%d, flags:%d}\n",
            head,
            entry->asked_size,
            head->lcount & MEMCC_DFSTACK_VALUE,
            head->ncount & MEMCC_DFSTACK_VALUE,
            head->lcount & MEMCC_DFSTACK_FLAGS);
    }
}

static void push_history(memcc_dfstack_t *dfstack, uint32_t asked_size) {
    struct memcc_dfmeta *meta = (struct memcc_dfmeta *)dfstack->last;
    head_history[head_history_count]->head = meta - meta->lcount;
    head_history[head_history_count]->asked_size = asked_size;
    ++head_history_count;

    print_history();
}