#pragma once

#include "main/util.h"
#include "sheap_defs.h"

struct schnk
{
    struct
    {
        size_t index : 8;
        size_t size : 24;
    };
    struct schnk *prev;
};
static_assert(sizeof(struct schnk) == 8, "");

struct sheap
{
    void **blocks;
    void *cur;
    struct schnk *last;
};

void sheap_purge(struct sheap *heap, void *edx);

void sheap_init(struct sheap *heap)
{
    heap->blocks = (void **)hr_calloc(SHEAP_MAX_BLOCKS, sizeof(void *));
    heap->blocks[0] = hr_malloc(SHEAP_BUFF_SIZE);
    heap->cur = heap->blocks[0];
    heap->last = NULL;
}

void sheap_init_fix(struct sheap *heap, void *edx)
{
    sheap_init(heap);
}

void sheap_init_var(struct sheap *heap, void *edx, size_t size)
{
    sheap_init(heap);
}

void *sheap_alloc(struct sheap *heap, void *edx, size_t size, size_t align)
{
    struct schnk *hdr;
    int i = heap->last ? heap->last->index : 0;
    void *alloc_end = VSUM(ROUND_UP(heap->cur, 4), sizeof(struct schnk) + size);
    void *block_end = VSUM(heap->blocks[i], SHEAP_BUFF_SIZE);

    if (alloc_end > block_end)
    {
        ++i;
        if (i == SHEAP_MAX_BLOCKS)
        {
            return NULL;
        }

        if (!heap->blocks[i])
        {
            heap->blocks[i] = hr_malloc(SHEAP_BUFF_SIZE);
            if (!heap->blocks[i])
            {
                return NULL;
            }
        }

        heap->cur = heap->blocks[i];
    }

    hdr = (struct schnk *)ROUND_UP(heap->cur, 4);

    hdr->index = i;
    hdr->size = size;
    hdr->prev = heap->last;

    heap->last = hdr++;
    heap->cur = VSUM(hdr, size);

    return hdr;
}

void sheap_free(struct sheap *heap, void *edx, void *addr)
{
    int i, j = 0;
    struct schnk *chunk = (struct schnk *)VSUB(addr, sizeof(struct schnk));

    if (addr && !(chunk->size & SHEAP_FREE))
    {
        for (chunk->size |= SHEAP_FREE;
             heap->last && (heap->last->size & SHEAP_FREE);
             heap->last = heap->last->prev)
        {
            i = heap->last->index + 2;
            if ((i != j) && heap->blocks[i])
            {
                hr_free(heap->blocks[i]);
                heap->blocks[i] = NULL;
                j = i;
            }
        }

        heap->cur = heap->last ? VSUM(heap->last,
                                      sizeof(struct schnk) + heap->last->size)
                               : heap->blocks[0];
    }
}

void sheap_purge(struct sheap *heap, void *edx)
{
    int i;

    for (i = 0; i < SHEAP_MAX_BLOCKS && heap->blocks[i]; i++)
    {
        hr_free(heap->blocks[i]);
    }
    hr_free(heap->blocks);
}

struct sheap *sheap_get_thread_local(void)
{
    static __declspec(thread) struct sheap *heap = NULL;

    if (!heap)
    {
        heap = (struct sheap *)hr_malloc(sizeof(struct sheap));
        sheap_init(heap);
    }

    return heap;
}
