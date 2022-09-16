#pragma once

#include "main/util.h"
#include "mheap_defs.h"
#include "mpool.h"

struct pool_data
{
    size_t item_size;
    size_t max_size;
};

// POOLS SIZE MUST BE A MULTIPLE OF POOL_ALIGN!!!
struct pool_data pool_desc[] = {
    {8u, 32u * MB},    {12u, 32u * MB},   {16u, 32u * MB},   {20u, 16u * MB},
    {24u, 16u * MB},   {28u, 16u * MB},   {32u, 16u * MB}, // 160
    {40u, 16u * MB},   {48u, 16u * MB},   {56u, 16u * MB},   {64u, 16u * MB},
    {80u, 128u * MB},  {96u, 128u * MB},  {112u, 32u * MB}, // 352
    {128u, 32u * MB},  {160u, 32u * MB},  {192u, 32u * MB},  {224u, 32u * MB},
    {256u, 16u * MB},  {320u, 32u * MB},  {384u, 16u * MB}, // 176
    {448u, 64u * MB},  {512u, 32u * MB},  {640u, 64u * MB},  {768u, 16u * MB},
    {896u, 16u * MB},  {1024u, 32u * MB}, {1280u, 64u * MB}, // 304
    {1536u, 64u * MB}, {1792u, 16u * MB}, {2048u, 32u * MB}, {2560u, 16u * MB},
    {3072u, 32u * MB}, {3584u, 32u * MB}, // 192
};

struct mheap
{
    struct mpool *pools_by_size[POOL_SIZE_ARRAY_LEN];
    struct mpool *pools_by_addr[POOL_ADDR_ARRAY_LEN];
    struct mpool *pools_by_indx[POOL_INDX_ARRAY_LEN];
    struct pool_data pool_desc[countof(pool_desc)];
};

void mheap_init_pools(struct mheap *heap);

void mheap_init(struct mheap *heap)
{
    hr_memset8(heap->pools_by_size, 0,
               POOL_SIZE_ARRAY_LEN * sizeof(struct mpool *));
    hr_memset8(heap->pools_by_addr, 0,
               POOL_ADDR_ARRAY_LEN * sizeof(struct mpool *));
    hr_memset8(heap->pools_by_indx, 0,
               POOL_INDX_ARRAY_LEN * sizeof(struct mpool *));
    mheap_init_pools(heap);
}

void mheap_init_pools(struct mheap *heap)
{
    size_t p;
    size_t i;
    for (p = 0; p < countof(pool_desc); p++)
    {
        struct pool_data *pd = &pool_desc[p];
        struct mpool *pool =
            (struct mpool *)hr_winapi_malloc(sizeof(struct mpool));
        mpool_init(pool, pd->item_size, pd->max_size, p);
        void *addr = mpool_setup(pool);
        if (p == 0)
        {
            heap->pools_by_size[0] = pool;
            heap->pools_by_size[1] = pool;
        }
        heap->pools_by_indx[p] = pool;
        for (i = pd->item_size >> 2u; i && !heap->pools_by_size[i]; i--)
        {
            heap->pools_by_size[i] = pool;
        }
        for (i = 0; i < ((pd->max_size + (POOL_ALIGN - 1u)) / POOL_ALIGN); i++)
        {
            heap->pools_by_addr[((uintptr_t)addr / POOL_ALIGN) + i] = pool;
        }
    }
}

struct mpool *mheap_pool_from_size(struct mheap *heap, size_t size)
{
    return heap->pools_by_size[(size + 3u) >> 2u];
}

struct mpool *mheap_pool_from_addr(struct mheap *heap, void *addr)
{
    return heap->pools_by_addr[(uintptr_t)addr / POOL_ALIGN];
}

struct mpool *mheap_pool_from_indx(struct mheap *heap, size_t indx)
{
    return heap->pools_by_indx[indx];
}

void *mheap_malloc(struct mheap *heap, size_t size)
{
    void *addr = NULL;
    size_t index;
    struct mpool *pool = mheap_pool_from_size(heap, size);

    if (addr = mpool_malloc(pool, size))
    {
        goto end;
    }

    index = mpool_get_index(pool);
    while (++index < countof(pool_desc))
    {
        pool = mheap_pool_from_indx(heap, index);
        if (addr = mpool_malloc(pool, size))
        {
            goto end;
        }
    }

end:
    return addr;
}

void *mheap_calloc(struct mheap *heap, size_t size)
{
    void *addr = mheap_malloc(heap, size);

    if (addr)
    {
        hr_memset32(addr, 0, (size + 3u) >> 2u);
    }

    return addr;
}

size_t mheap_mem_size(struct mheap *heap, void *addr)
{
    size_t ret = 0;
    struct mpool *pool = mheap_pool_from_addr(heap, addr);

    if (!pool)
    {
        goto end;
    }

    ret = mpool_mem_size(pool, addr);

end:
    return ret;
}

int mheap_free(struct mheap *heap, void *addr)
{
    int ret = 0;
    struct mpool *pool = mheap_pool_from_addr(heap, addr);

    if (!pool)
    {
        goto end;
    }

    ret = 1;

    mpool_free(pool, addr);

end:
    return ret;
}
