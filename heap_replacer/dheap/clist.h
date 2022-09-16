#pragma once

#include "cnode.h"
#include "main/heap_replacer.h"
#include "mcell.h"

struct cnode;
struct mcell;

struct clist
{
    struct cnode head;
    struct cnode tail;
};

void clist_init(struct clist *list)
{
    list->head = (struct cnode){&list->tail, NULL, 0xffffffffu};
    list->tail = (struct cnode){NULL, &list->head, 0xffffffffu};
}

struct cnode *clist_get_head(struct clist *list)
{
    return list->head.next;
}

struct cnode *clist_get_tail(struct clist *list)
{
    return list->tail.prev;
}

void clist_add_head(struct clist *list, struct cnode *node)
{
    cnode_link(node, &list->head, list->head.next);
}

void clist_add_tail(struct clist *list, struct cnode *node)
{
    cnode_link(node, list->tail.prev, &list->tail);
}

void clist_insert_before(struct clist *list, struct cnode *node,
                         struct cnode *pos)
{
    cnode_link(node, pos->prev, pos);
}

void clist_insert_after(struct clist *list, struct cnode *node,
                        struct cnode *pos)
{
    cnode_link(node, pos, pos->next);
}

void clist_remove_node(struct clist *list, struct cnode *__restrict node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = NULL;
    node->prev = NULL;
}

int clist_is_empty(struct clist *list)
{
    return list->head.next == &list->tail;
}
