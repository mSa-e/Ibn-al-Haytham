#ifndef Haitham_DICTIONARY_H
#define Haitham_DICTIONARY_H
#include "../Trees/BinarySearchTree.h"
#include "../Lists/LinkedList.h"
#include "Entry.h"
struct Dictionary { struct BinarySearchTree binary_search_tree; struct LinkedList keys; int(*key_compare)(void*,void*); void(*insert)(struct Dictionary*,void*,unsigned long,void*,unsigned long); void*(*search)(struct Dictionary*,void*,unsigned long); };
struct Dictionary dictionary_constructor(int (*compare)(void*,void*));
void dictionary_destructor(struct Dictionary *dictionary);
int compare_string_keys(void *a,void *b);
#endif
