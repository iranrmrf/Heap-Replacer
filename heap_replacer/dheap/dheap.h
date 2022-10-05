#pragma once

#include "main/util.h"

#include "cdesc.h"
#include "clist.h"
#include "dheap_defs.h"
#include "locks/rlock.h"
#include "mcell.h"

struct dheap
{
    struct clist size_list;
    struct clist addr_list[DHEAP_BLOCK_COUNT];
    struct mcell *size_array[DHEAP_CELLS_PER_BLOCK];
    struct mcell **addr_array[DHEAP_BLOCK_COUNT];
    struct mcell **used_cells[DHEAP_BLOCK_COUNT];
    struct cdesc block_desc[DHEAP_BLOCK_COUNT];
    struct rlock lock;
};

size_t dheap_get_block_index(struct dheap *heap, void *addr);
struct mcell *dheap_create_new_block(struct dheap *heap);
void dheap_add_free_cell(struct dheap *heap, struct mcell *cell);

void dheap_init(struct dheap *heap)
{
    clist_init(&heap->size_list);
    rlock_init(&heap->lock);
}

size_t dheap_get_size_index_from_size(struct dheap *heap, size_t size)
{
    return (size / DHEAP_CELL_SIZE) - 1u;
}

size_t dheap_get_size_index_from_cell(struct dheap *heap, struct mcell *cell)
{
    return dheap_get_size_index_from_size(heap, cell->desc.size);
}

size_t dheap_get_addr_index_from_cell(struct dheap *heap, struct mcell *cell)
{
    return USUB(cell->desc.addr, heap->block_desc[cell->desc.index].addr) /
           DHEAP_CELL_SIZE;
}

void dheap_add_cell_to_size_array(struct dheap *heap, struct mcell *cell)
{
    struct cnode *curr;
    struct mcell *elem;

    if ((elem = heap->size_array[cell->size_node.array_index]) &&
        (cell->desc.size == elem->desc.size))
    {
        return;
    }

    for (curr = cell->size_node.prev;
         cnode_is_valid(curr) &&
         mcell_is_smaller(cell, get_mcell_safe(curr, size_node));
         curr = curr->prev)
    {
    }

    hr_memset32(&heap->size_array[curr->array_index + 1u], (DWORD)cell,
                cell->size_node.array_index - curr->array_index);
}

void dheap_add_cell_to_addr_array(struct dheap *heap, struct mcell *cell)
{
    struct cnode *curr;

    for (curr = cell->addr_node.prev;
         cnode_is_valid(curr) &&
         mcell_is_before(cell, get_mcell_safe(curr, addr_node));
         curr = curr->prev)
    {
    }

    hr_memset32(&heap->addr_array[cell->desc.index][curr->array_index + 1u],
                (DWORD)cell, cell->addr_node.array_index - curr->array_index);
}

void dheap_rmv_cell_from_size_array(struct dheap *heap, struct mcell *cell)
{
    struct cnode *curr = cell->size_node.prev;

    if (cell == heap->size_array[curr->array_index + 1u])
    {
        hr_memset32(&heap->size_array[curr->array_index + 1u],
                    (DWORD)get_mcell(cell->size_node.next, size_node),
                    cell->size_node.array_index - curr->array_index);
    }
}

void dheap_rmv_cell_from_addr_array(struct dheap *heap, struct mcell *cell)
{
    struct cnode *curr = cell->addr_node.prev;

    hr_memset32(&heap->addr_array[cell->desc.index][curr->array_index + 1u],
                (DWORD)get_mcell(cell->addr_node.next, addr_node),
                cell->addr_node.array_index - curr->array_index);
}

void dheap_add_cell_to_size_list(struct dheap *heap, struct mcell *cell)
{
    struct mcell *elem;
    size_t index = dheap_get_size_index_from_cell(heap, cell);

    if (elem = heap->size_array[index])
    {
        if (elem->desc.size == cell->desc.size)
        {
            clist_insert_after(&heap->size_list, &cell->size_node,
                               &elem->size_node);
        }
        else
        {
            clist_insert_before(&heap->size_list, &cell->size_node,
                                &elem->size_node);
        }
        goto end;
    }

    clist_add_tail(&heap->size_list, &cell->size_node);

end:
    cell->size_node.array_index = index;
}

void dheap_add_cell_to_addr_list(struct dheap *heap, struct mcell *cell)
{
    struct mcell *elem;
    size_t index = dheap_get_addr_index_from_cell(heap, cell);

    if (elem = heap->addr_array[cell->desc.index][index])
    {
        clist_insert_before(&heap->addr_list[cell->desc.index],
                            &cell->addr_node, &elem->addr_node);
        goto end;
    }

    clist_add_tail(&heap->addr_list[cell->desc.index], &cell->addr_node);

end:
    cell->addr_node.array_index = index;
}

void dheap_put_used_cell(struct dheap *heap, struct mcell *cell)
{
    size_t addr_index = dheap_get_addr_index_from_cell(heap, cell);
    heap->used_cells[cell->desc.index][addr_index] = cell;
}

struct mcell *dheap_rmv_used_cell(struct dheap *heap, void *addr, size_t index)
{
    size_t addr_index =
        USUB(addr, heap->block_desc[index].addr) / DHEAP_CELL_SIZE;
    return InterlockedExchangePointer(&heap->used_cells[index][addr_index],
                                      NULL);
}

struct mcell *dheap_get_used_cell(struct dheap *heap, void *addr, size_t index)
{
    size_t addr_index =
        USUB(addr, heap->block_desc[index].addr) / DHEAP_CELL_SIZE;
    return heap->used_cells[index][addr_index];
}

void dheap_rmv_free_cell(struct dheap *heap, struct mcell *cell)
{
    dheap_rmv_cell_from_size_array(heap, cell);
    dheap_rmv_cell_from_addr_array(heap, cell);

    clist_remove_node(&heap->size_list, &cell->size_node);
    clist_remove_node(&heap->addr_list[cell->desc.index], &cell->addr_node);
}

void dheap_add_free_cell(struct dheap *heap, struct mcell *cell)
{
    rlock_lock(&heap->lock);

    dheap_add_cell_to_addr_list(heap, cell);

    struct mcell *__restrict temp;
    if ((temp = get_mcell(cell->addr_node.prev, addr_node)) &&
        mcell_precedes(temp, cell))
    {
        dheap_rmv_free_cell(heap, temp);
        cell->desc.addr = temp->desc.addr;
        cell->desc.size += temp->desc.size;
        cell->addr_node.array_index = temp->addr_node.array_index;
        hr_free(temp);
    }
    if ((temp = get_mcell(cell->addr_node.next, addr_node)) &&
        mcell_precedes(cell, temp))
    {
        dheap_rmv_free_cell(heap, temp);
        cell->desc.size += temp->desc.size;
        hr_free(temp);
    }

    dheap_add_cell_to_addr_array(heap, cell);
    dheap_add_cell_to_size_list(heap, cell);
    dheap_add_cell_to_size_array(heap, cell);

    rlock_unlock(&heap->lock);
}

struct mcell *dheap_get_free_cell(struct dheap *heap, size_t size)
{
    size_t rsize, index;

    rsize = ROUND_UP(size, DHEAP_CELL_SIZE);
    index = dheap_get_size_index_from_size(heap, rsize);

    struct mcell *cell;
    struct mcell *split;

    rlock_lock(&heap->lock);

    cell = heap->size_array[index];
    if (!cell)
    {
        cell = dheap_create_new_block(heap);
        if (!cell)
        {
            HR_LOG("failed to create new block");
            goto end;
        }
        dheap_add_free_cell(heap, cell);
    }

    if ((cell->desc.size - rsize) < DHEAP_MIN_CELL_SIZE)
    {
        dheap_rmv_free_cell(heap, cell);
    }
    else
    {
        dheap_rmv_cell_from_size_array(heap, cell);
        clist_remove_node(&heap->size_list, &cell->size_node);

        split = mcell_split(cell, rsize);

        dheap_add_cell_to_size_list(heap, cell);
        dheap_add_cell_to_size_array(heap, cell);

        cell = split;
    }

end:
    rlock_unlock(&heap->lock);
    return cell;
}

size_t dheap_get_block_index(struct dheap *heap, void *addr)
{
    size_t index = DHEAP_BLOCK_COUNT;

    for (size_t i = 0u; i < DHEAP_BLOCK_COUNT; i++)
    {
        if (cdesc_is_in_range(&heap->block_desc[i], addr))
        {
            index = i;
            goto end;
        }
    }

end:
    return index;
}

size_t dheap_next_free_block_index(struct dheap *heap)
{
    size_t index = DHEAP_BLOCK_COUNT;

    for (size_t i = 0u; i < DHEAP_BLOCK_COUNT; i++)
    {
        if (!heap->block_desc[i].addr)
        {
            index = i;
            goto end;
        }
    }

end:
    return index;
}

struct mcell *dheap_create_new_block(struct dheap *heap)
{
    void *addr;
    struct mcell *cell = NULL;
    size_t index = dheap_next_free_block_index(heap);

    if (index == DHEAP_BLOCK_COUNT)
    {
        HR_LOG("no more blocks available");
        goto end;
    }

    addr = hr_winapi_malloc(DHEAP_BLOCK_SIZE);
    if (!addr)
    {
        HR_LOG("failed to allocate new block");
        goto end;
    }

    HR_LOG("new block %d allocated at 0x%08X", index, (DWORD)addr);

    heap->block_desc[index] = (struct cdesc){addr, DHEAP_BLOCK_SIZE, index};
    clist_init(&heap->addr_list[index]);

    heap->addr_array[index] = (struct mcell **)hr_winapi_calloc(
        DHEAP_CELLS_PER_BLOCK, sizeof(struct mcell *));
    if (!heap->addr_array[index])
    {
        HR_LOG("failed to allocate new addr array");
        goto end;
    }

    heap->used_cells[index] = (struct mcell **)hr_winapi_calloc(
        DHEAP_CELLS_PER_BLOCK, sizeof(struct mcell *));
    if (!heap->used_cells[index])
    {
        HR_LOG("failed to allocate new used cells array");
        goto end;
    }

    cell = (struct mcell *)hr_malloc(sizeof(struct mcell));
    if (!cell)
    {
        HR_LOG("failed to allocate new cell");
        goto end;
    }
    cell->desc = (struct cdesc){addr, DHEAP_BLOCK_SIZE, index};

end:
    return cell;
}

void *dheap_malloc(struct dheap *heap, size_t size)
{
    struct mcell *cell = dheap_get_free_cell(heap, size);
    void *addr = NULL;

    if (!cell)
    {
        goto end;
    }

    dheap_put_used_cell(heap, cell);
    addr = cell->desc.addr;

end:
    return addr;
}

void *dheap_calloc(struct dheap *heap, size_t size)
{
    void *addr = dheap_malloc(heap, size);

    if (addr)
    {
        hr_memset32(addr, 0, (size + 3u) >> 2u);
    }

    return addr;
}

size_t dheap_mem_size(struct dheap *heap, void *addr)
{
    size_t index = dheap_get_block_index(heap, addr);
    struct mcell *cell;
    size_t size = 0;

    if (index == DHEAP_BLOCK_COUNT)
    {
        goto end;
    }

    cell = dheap_get_used_cell(heap, addr, index);
    size = cell->desc.size;

end:
    return size;
}

int dheap_free(struct dheap *heap, void *addr)
{
    size_t index = dheap_get_block_index(heap, addr);
    struct mcell *cell;
    int ret = 0;

    if (index == DHEAP_BLOCK_COUNT)
    {
        goto end;
    }

    ret = 1;

    if (cell = dheap_rmv_used_cell(heap, addr, index))
    {
        dheap_add_free_cell(heap, cell);
    }

end:
    return ret;
}
