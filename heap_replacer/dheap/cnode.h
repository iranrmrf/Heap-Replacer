#pragma once

#define get_mcell_safe(node, type)                                             \
    ((struct mcell *)(((DWORD)node - offsetof(struct mcell, type))))

#define get_mcell(node, type)                                                  \
    ((struct mcell *)((DWORD)get_mcell_safe(node, type) &                      \
                      (!cnode_is_valid(node) - 1)))

struct cnode
{
    struct cnode *next;
    struct cnode *prev;
    size_t array_index;
};

void cnode_link(struct cnode *node, struct cnode *prev, struct cnode *next)
{
    node->prev = prev;
    prev->next = node;
    node->next = next;
    next->prev = node;
}

int cnode_is_valid(struct cnode *node)
{
    return !!node->next & !!node->prev;
}
