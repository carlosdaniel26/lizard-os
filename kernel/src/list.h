#pragma once

struct ListHead {
    struct ListHead *next;
    struct ListHead *prev;
};

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

static inline void ListAdd(struct ListHead *new, struct ListHead *head)
{
    __ListAdd(new, head, head->next);
}

static inline void ListAddTail(struct ListHead *new, struct ListHead *head)
{
    __ListAdd(new, head->prev, head);
}

static inline void __ListDel(
    struct ListHead *prev,
    struct ListHead *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void ListDel(struct ListHead *entry)
{
    __ListDel(entry->prev, entry->next);
    entry->next = entry->prev = nullptr; /* ou NULL se C */
}

#define ListForEach(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)