#include "Queue.h"
static void push_q(struct Queue*q,void*d,unsigned long s){if(q)q->list.insert(&q->list,q->list.length,d,s);}
static void *peek_q(struct Queue*q){return q?q->list.retrieve(&q->list,0):0;}
static void pop_q(struct Queue*q){if(q)q->list.remove(&q->list,0);}
struct Queue queue_constructor(void){struct Queue q={0};q.list=linked_list_constructor();q.push=push_q;q.peek=peek_q;q.pop=pop_q;return q;}
void queue_destructor(struct Queue*q){if(q)linked_list_destructor(&q->list);}
