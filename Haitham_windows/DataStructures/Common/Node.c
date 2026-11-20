#include "Node.h"
#include <stdlib.h>
#include <string.h>
struct Node node_constructor(const void *data, unsigned long size) {
    struct Node n = {0};
    if (!data || !size) return n;
    n.data = malloc((size_t)size);
    if (!n.data) return n;
    memcpy(n.data, data, (size_t)size);
    return n;
}
void node_destructor(struct Node *node) {
    if (!node) return;
    free(node->data); node->data = NULL;
    free(node);
}
