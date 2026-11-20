#ifndef Haitham_QUEUE_H
#define Haitham_QUEUE_H
#include "LinkedList.h"
struct Queue { struct LinkedList list; void (*push)(struct Queue*,void*,unsigned long); void *(*peek)(struct Queue*); void (*pop)(struct Queue*); };
struct Queue queue_constructor(void);
void queue_destructor(struct Queue *queue);
#endif
