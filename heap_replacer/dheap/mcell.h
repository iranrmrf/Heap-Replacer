#pragma once

#include "cdesc.h"
#include "cnode.h"
#include "main/heap_replacer.h"

struct cnode;
struct cdesc;

struct mcell
{
    struct cdesc desc;
    struct cnode size_node;
    struct cnode addr_node;
};

int mcell_precedes(struct mcell *cell, struct mcell *other)
{
    return cdesc_get_end(&cell->desc) == other->desc.addr;
}

struct mcell *mcell_split(struct mcell *cell, size_t size)
{
    struct mcell *__restrict new_cell =
        (struct mcell *)hr_malloc(sizeof(struct mcell));

    cell->desc.size -= size;

    new_cell->desc =
        (struct cdesc){cdesc_get_end(&cell->desc), size, cell->desc.index};

    return new_cell;
}

int mcell_is_smaller(struct mcell *cell, struct mcell *other)
{
    return cell->desc.size < other->desc.size;
}

int mcell_is_before(struct mcell *cell, struct mcell *other)
{
    return cell->desc.addr < other->desc.addr;
}
