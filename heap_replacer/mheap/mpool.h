#pragma once

#include "locks/nlock.h"
#include "main/util.h"
#include "mheap.h"
#include "mheap_defs.h"

struct cell
{
    struct cell *next;
};

struct mpool
{
    size_t item_size;
    size_t max_size;
    size_t max_cell_count;

    size_t block_count;
    size_t cells_per_block;

    void *bgn;
    void *cur;
    void *end;

    struct cell *free_cells;
    struct cell *next_free;

    size_t index;

    struct nlock lock;
};

void mpool_init(struct mpool *pool, size_t item_size, size_t max_size,
                size_t index)
{
    pool->item_size = item_size;
    pool->max_size = max_size;

    pool->bgn = NULL;
    pool->cur = NULL;
    pool->end = NULL;

    pool->block_count = pool->max_size / POOL_BLOCK_SIZE;
    pool->cells_per_block = POOL_BLOCK_SIZE / pool->item_size;

    pool->max_cell_count = pool->block_count * pool->cells_per_block;

    pool->free_cells = (struct cell *)hr_winapi_calloc(pool->max_cell_count,
                                                       sizeof(struct cell));
    pool->next_free = pool->free_cells;

    pool->index = index;

    nlock_init(&pool->lock);
}

void *mpool_setup(struct mpool *pool)
{
    size_t i = 0;

    while (!pool->bgn)
    {
        pool->bgn = VirtualAlloc((void *)(++i * POOL_ALIGN), pool->max_size,
                                 MEM_RESERVE, PAGE_READWRITE);
        if (i == 0xff)
        {
            i = 0;
        }
    }
    pool->cur = pool->bgn;
    pool->end = VSUM(pool->bgn, pool->max_size);

    return pool->bgn;
}

void mpool_setup_new_block(struct mpool *pool)
{
    pool->cur =
        VirtualAlloc(pool->cur, POOL_BLOCK_SIZE, MEM_COMMIT, PAGE_READWRITE);
    size_t bank_offset =
        USUB(pool->cur, pool->bgn) / POOL_BLOCK_SIZE * pool->cells_per_block;

    pool->free_cells[bank_offset].next = NULL;
    for (size_t i = 0u; i < pool->cells_per_block - 1u; i++)
    {
        pool->free_cells[bank_offset + i + 1u].next =
            &pool->free_cells[bank_offset + i];
    }
    pool->next_free =
        &pool->free_cells[bank_offset + pool->cells_per_block - 1u];

    pool->cur = VSUM(pool->cur, POOL_BLOCK_SIZE);
}

void *mpool_free_ptr_to_real(struct mpool *pool, void *addr)
{
    return VSUM(pool->bgn,
                ((USUB(addr, pool->free_cells) >> 2u) * pool->item_size));
}

void *mpool_real_to_free_ptr(struct mpool *pool, void *addr)
{
    return VSUM(((USUB(addr, pool->bgn) / pool->item_size) << 2u),
                pool->free_cells);
}

int mpool_is_in_range(struct mpool *pool, void *addr)
{
    return ((pool->bgn <= addr) & (addr < pool->end));
}

void *mpool_malloc(struct mpool *pool, size_t size)
{
    void *addr;
    struct cell *old_free;

    nlock_lock(&pool->lock);

    if (!pool->next_free->next)
    {
        if (pool->cur == pool->end)
        {
            nlock_unlock(&pool->lock);
            /* HR_LOG("pool %d oom", pool->item_size); */
            return NULL;
        }
        mpool_setup_new_block(pool);
    }

    __assume(pool->next_free != NULL);

    old_free = pool->next_free;
    pool->next_free = pool->next_free->next;

    nlock_unlock(&pool->lock);

    old_free->next = NULL;
    addr = mpool_free_ptr_to_real(pool, old_free);

    return addr;
}

size_t mpool_mem_size(struct mpool *pool, void *addr)
{
    return pool->item_size;
}

void mpool_free(struct mpool *pool, void *addr)
{
    struct cell *c = (struct cell *)mpool_real_to_free_ptr(pool, addr);

    nlock_lock(&pool->lock);

    if (!c->next)
    {
        c->next = pool->next_free;
        pool->next_free = c;
    }

    nlock_unlock(&pool->lock);
}

size_t mpool_get_index(struct mpool *pool)
{
    return pool->index;
}
