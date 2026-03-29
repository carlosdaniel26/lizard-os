#pragma once

#include <types.h>

struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

#define LIST_HEAD_INIT(name)                                                                                 \
    {                                                                                                        \
        &(name), &(name)                                                                                     \
    }

#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

static inline void InitListHead(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void __ListAdd(struct list_head *new, struct list_head *prev, struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
    __ListAdd(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    __ListAdd(new, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = entry->prev = NULL;
}

static inline void list_move_tail(struct list_head *entry, struct list_head *head)
{
    list_del(entry);
    list_add_tail(entry, head);
}

static inline void list_move(struct list_head *entry, struct list_head *head)
{
    list_del(entry);
    list_add(entry, head);
}

static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

static inline struct list_head *list_next(const struct list_head *pos, const struct list_head *head)
{
    struct list_head *next = pos->next;
    if (next == head)
    {
        next = next->next;
    }

    return next;
}

#define list_for_each(pos, tmp, head)                                                                        \
    for (pos = (head)->next, tmp = pos->next; pos != (head); pos = tmp, tmp = pos->next)

#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

#define list_first_entry(head, type, member) container_of((head)->next, type, member)