#pragma once

#include "dheap_defs.h"

struct cdesc
{
    void *addr;
    struct
    {
        size_t size : DHEAP_CELL_SIZE_BITS;
        size_t index : 32 - DHEAP_CELL_SIZE_BITS;
    };
};

void *cdesc_get_end(struct cdesc *desc)
{
    return VSUM(desc->addr, desc->size);
}

int cdesc_is_in_range(struct cdesc *desc, void *addr)
{
    return ((desc->addr <= addr) & (addr < cdesc_get_end(desc)));
}
