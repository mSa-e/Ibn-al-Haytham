#include "LinkedList.h"
#include <stdlib.h>

static struct Node *at(struct LinkedList *l, int i)
{
    struct Node *n;
    if (!l || i < 0 || i >= l->length) return NULL;
    n = l->head;
    while (i-- && n) n = n->next;
    return n;
}

static void ins(struct LinkedList *l, int i, void *d, unsigned long z)
{
    struct Node *n;
    if (!l || !d || !z || i < 0 || i > l->length) return;
    n = malloc(sizeof(*n));
    if (!n) return;
    *n = node_constructor(d, z);
    if (!n->data) { free(n); return; }

    if (i == l->length) {                       /* append (also covers empty list) */
        n->previous = l->tail;
        if (l->tail) l->tail->next = n; else l->head = n;
        l->tail = n;
    } else if (i == 0) {                        /* prepend; list is non-empty here */
        n->next = l->head;
        l->head->previous = n;
        l->head = n;
    } else {                                    /* middle */
        struct Node *p = at(l, i - 1);
        n->next = p->next;
        n->previous = p;
        p->next->previous = n;
        p->next = n;
    }
    l->length++;
}

static void rem(struct LinkedList *l, int i)
{
    struct Node *n = at(l, i);
    if (!n) return;
    if (n->previous) n->previous->next = n->next; else l->head = n->next;
    if (n->next) n->next->previous = n->previous; else l->tail = n->previous;
    free(n->data);
    free(n);
    l->length--;
}

static void *get(struct LinkedList *l, int i)
{
    struct Node *n = at(l, i);
    return n ? n->data : NULL;
}

/* Bubble sort over the node chain (O(n^2), was O(n^3) because of at() in the inner loop). */
static void sort_list(struct LinkedList *l, int (*cmp)(void *, void *))
{
    if (!l || !cmp) return;
    for (int i = 0; i < l->length - 1; i++) {
        int swapped = 0;
        struct Node *a = l->head;
        for (int j = 0; j < l->length - 1 - i && a && a->next; j++, a = a->next) {
            struct Node *b = a->next;
            if (cmp(a->data, b->data) > 0) {
                void *t = a->data; a->data = b->data; b->data = t;
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
}

static short search_list(struct LinkedList *l, void *q, int (*cmp)(void *, void *))
{
    if (!l || !cmp) return 0;
    for (struct Node *n = l->head; n; n = n->next)
        if (cmp(n->data, q) == 0) return 1;
    return 0;
}

struct LinkedList linked_list_constructor(void)
{
    struct LinkedList l = {0};
    l.insert = ins; l.remove = rem; l.retrieve = get; l.sort = sort_list; l.search = search_list;
    return l;
}

void linked_list_destructor(struct LinkedList *l)
{
    if (!l) return;
    while (l->length) rem(l, 0);
}
