#pragma once

#include <stddef.h>

typedef struct ListHead {
    struct ListHead *next;
    struct ListHead *prev;
} ListHead;

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct ListHead name = LIST_HEAD_INIT(name)

static inline void InitListHead(struct ListHead *list)
{
    list->next = list;
    list->prev = list;
}

static inline void __ListAdd(
    struct ListHead *new,
    struct ListHead *prev,
    struct ListHead *next)
{
    next->prev = new;
    new->next  = next;
    new->prev  = prev;
    prev->next = new;
}

static inline void list_add(struct ListHead *new, struct ListHead *head)
{
    __ListAdd(new, head, head->next);
}

static inline void list_add_tail(struct ListHead *new, struct ListHead *head)
{
    __ListAdd(new, head->prev, head);
}

static inline void __list_del(
    struct ListHead *prev,
    struct ListHead *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct ListHead *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = entry->prev = NULL;
}

static inline void list_move_tail(struct ListHead *entry, struct ListHead *head)
{
    list_del(entry);
    list_add_tail(entry, head);
}

static inline void list_move(struct ListHead *entry, struct ListHead *head)
{
    list_del(entry);
    list_add(entry, head);
}


#define list_for_each(pos, tmp, head) \
    for (pos = (head)->next, tmp = pos->next; pos != (head); \
         pos = tmp, tmp = pos->next)

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define list_first_entry(head, type, member) \
    container_of((head)->next, type, member)