#ifndef Haitham_ENTRY_H
#define Haitham_ENTRY_H
struct Entry { void *key; void *value; int (*compare)(void *, void *); };
struct Entry entry_constructor(const void *key, unsigned long key_size, const void *value, unsigned long value_size);
void entry_destructor(struct Entry *entry);
#endif
