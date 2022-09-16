#pragma once

#include "locks/nlock.h"
#include "main/util.h"
#include "mheap.h"
#include "mheap_defs.h"

struct cell
{
    struct cell *next;
};

union pool_state {
    struct
    {
        struct cell *next_free;
        __int32 counter;
    };
    __int64 state;
};

struct mpool
{
    size_t item_size;
    size_t max_size;
    size_t cell_count;
    size_t max_cell_count;

    size_t block_count;
    size_t cells_per_block;

    void *bgn;
    void *cur;
    void *end;

    struct cell *free_cells;

    struct cell *saved_next;
    int busy;

    union {
        struct
        {
            struct cell *next_free;
            __int32 counter;
        };
        __int64 state;
    };

    size_t index;

    struct nlock lock;
};

struct cell *mpool_setup_new_block(struct mpool *pool);

void mpool_init(struct mpool *pool, size_t item_size, size_t max_size,
                size_t index)
{
    pool->item_size = item_size;
    pool->max_size = max_size;
    pool->cell_count = 0;

    pool->bgn = NULL;
    pool->cur = NULL;
    pool->end = NULL;

    pool->block_count = pool->max_size / POOL_BLOCK_SIZE;
    pool->cells_per_block = POOL_BLOCK_SIZE / pool->item_size;

    pool->max_cell_count = pool->block_count * pool->cells_per_block;

    pool->free_cells =
        (struct cell *)winapi_calloc(pool->max_cell_count, sizeof(struct cell));
    pool->next_free = NULL;

    pool->index = index;

    nlock_init(&pool->lock);
}

void *mpool_setup(struct mpool *pool)
{
    size_t i = 0;

    while (!pool->bgn)
    {
        pool->bgn = VirtualAlloc((void *)(++i * POOL_ALIGN), pool->max_size,
                                 MEM_RESERVE, PAGE_READWRITE); // large pages
        if (i == 0xff)
        {
            i = 0;
        }
    }
    pool->cur = pool->bgn;
    pool->end = VSUM(pool->bgn, pool->max_size);

    pool->saved_next = mpool_setup_new_block(pool);
    pool->busy = 0;

    return pool->bgn;
}

struct cell *mpool_setup_new_block(struct mpool *pool)
{
    pool->cur =
        VirtualAlloc(pool->cur, POOL_BLOCK_SIZE, MEM_COMMIT, PAGE_READWRITE);
    size_t bank_offset =
        USUB(pool->cur, pool->bgn) / POOL_BLOCK_SIZE * pool->cells_per_block;

    if (pool->item_size == 196)
    {
        printf("bank_offset: %d\n", bank_offset);
    }

    for (size_t i = 0u; i < pool->cells_per_block - 1u; i++)
    {
        pool->free_cells[bank_offset + i].next =
            &pool->free_cells[bank_offset + i + 1u];
    }

    pool->free_cells[bank_offset + pool->cells_per_block - 1u].next = NULL;

    pool->cur = VSUM(pool->cur, POOL_BLOCK_SIZE);

    return &pool->free_cells[bank_offset];
}

void *mpool_translate_to_real(struct mpool *pool, void *addr)
{
    return VSUM(pool->bgn,
                ((USUB(addr, pool->free_cells) >> 2u) * pool->item_size));
}

void *mpool_translate_from_real(struct mpool *pool, void *addr)
{
    return VSUM(((USUB(addr, pool->bgn) / pool->item_size) << 2u),
                pool->free_cells);
}

int mpool_is_in_range(struct mpool *pool, void *addr)
{
    return ((pool->bgn <= addr) & (addr < pool->end));
}

void *mpool_malloc(struct mpool *pool)
{
    union pool_state os, ns;

    struct cell *old_free;

    do
    {
        if (InterlockedCompareExchange(&pool->next_free, NULL, NULL) == NULL)
        {
            if (InterlockedCompareExchange(&pool->busy, 1, 0) == 0)
            {
                do
                {
                    os.state = pool->state;
                    (pool->saved_next + pool->cells_per_block - 1u)->next =
                        os.next_free;

                    ns.next_free = pool->saved_next;
                    ns.counter = os.counter + 1;
                } while (InterlockedCompareExchange64(&pool->state, ns.state,
                                                      os.state) != os.state);

                pool->saved_next = mpool_setup_new_block(pool);
                pool->busy = 0;
            }
            else
            {
                printf("spin\n");
                Sleep(0u);
                continue;
            }
        }

        os.state = pool->state;
        old_free = os.next_free;

        ns.next_free = old_free->next;
        ns.counter = os.counter + 1;

    } while (InterlockedCompareExchange64(&pool->state, ns.state, os.state) !=
             os.state);

    old_free->next = NULL;

    void *addr = mpool_translate_to_real(pool, old_free);

    return addr;
}

void *mpool_calloc(struct mpool *pool)
{
    void *addr = mpool_malloc(pool);
    if (addr)
    {
        cmemset32(addr, 0, pool->item_size >> 2u);
    }
    return addr;
}

size_t mpool_mem_size(struct mpool *pool)
{
    return pool->item_size;
}

void mpool_free(struct mpool *pool, void *addr)
{
    struct cell *c = (struct cell *)mpool_translate_from_real(pool, addr);

    union pool_state os, ns;

    if (InterlockedCompareExchangePointer(&c->next, NULL, 0xffffffff) == NULL)
    {
        do
        {
            os.state = pool->state;
            c->next = os.next_free;

            ns.next_free = c;
            ns.counter = os.counter + 1;

        } while (InterlockedCompareExchange64(&pool->state, ns.state,
                                              os.state) != os.state);
    }
    else
    {
        printf("invalid free\n");
    }
}

size_t mpool_get_index(struct mpool *pool)
{
    return pool->index;
}
