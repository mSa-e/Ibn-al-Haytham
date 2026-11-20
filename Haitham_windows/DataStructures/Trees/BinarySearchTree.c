#include "BinarySearchTree.h"
#include <stdlib.h>
static void *search_bst(struct BinarySearchTree *t, void *q) {
    struct Node *p=t?t->head:NULL;
    while(p){int c=t->compare(p->data,q); if(!c)return p->data; p=c>0?p->previous:p->next;}
    return NULL;
}
static void insert_bst(struct BinarySearchTree *t, void *data, unsigned long size) {
    if(!t||!data||!size||!t->compare)return;
    struct Node *n=malloc(sizeof(*n)); if(!n)return;
    *n=node_constructor(data,size); if(!n->data){free(n);return;}
    if(!t->head){t->head=n;return;}
    struct Node *p=t->head;
    for(;;){int c=t->compare(p->data,data); if(!c){free(n->data);free(n);return;}
        struct Node **slot=c>0?&p->previous:&p->next;
        if(!*slot){*slot=n;return;} p=*slot;
    }
}
static void destroy_nodes(struct Node *n){if(!n)return;destroy_nodes(n->previous);destroy_nodes(n->next);free(n->data);free(n);}
struct BinarySearchTree binary_search_tree_constructor(int (*cmp)(void *,void *)){
    struct BinarySearchTree t={0};t.compare=cmp;t.search=search_bst;t.insert=insert_bst;return t;
}
void binary_search_tree_destructor(struct BinarySearchTree *t){if(t){destroy_nodes(t->head);t->head=NULL;}}
