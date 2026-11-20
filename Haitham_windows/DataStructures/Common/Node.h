#ifndef Haitham_NODE_H
#define Haitham_NODE_H
struct Node { void *data; struct Node *next, *previous; };
struct Node node_constructor(const void *data, unsigned long size);
void node_destructor(struct Node *node);
#endif
