#ifndef Haitham_LINKEDLIST_H
#define Haitham_LINKEDLIST_H
#include "../Common/Node.h"

struct LinkedList {
    struct Node *head;
    struct Node *tail;      /* makes appending (and therefore Queue::push) O(1) */
    int length;
    void (*insert)(struct LinkedList *, int, void *, unsigned long);
    void (*remove)(struct LinkedList *, int);
    void *(*retrieve)(struct LinkedList *, int);
    void (*sort)(struct LinkedList *, int (*)(void *, void *));
    short (*search)(struct LinkedList *, void *, int (*)(void *, void *));
};

struct LinkedList linked_list_constructor(void);
void linked_list_destructor(struct LinkedList *list);
#endif
