#ifndef Haitham_BST_H
#define Haitham_BST_H
#include "../Common/Node.h"
struct BinarySearchTree {
    struct Node *head;
    int (*compare)(void *, void *);
    void *(*search)(struct BinarySearchTree *, void *);
    void (*insert)(struct BinarySearchTree *, void *, unsigned long);
};
struct BinarySearchTree binary_search_tree_constructor(int (*compare)(void *, void *));
void binary_search_tree_destructor(struct BinarySearchTree *tree);
#endif
