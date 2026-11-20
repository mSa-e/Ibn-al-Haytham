#include "Entry.h"
#include <stdlib.h>
#include <string.h>
struct Entry entry_constructor(const void *key, unsigned long ks, const void *value, unsigned long vs) {
    struct Entry e = {0};
    if (!key || !value || !ks || !vs) return e;
    e.key = malloc((size_t)ks); e.value = malloc((size_t)vs);
    if (!e.key || !e.value) { free(e.key); free(e.value); e.key=e.value=NULL; return e; }
    memcpy(e.key,key,(size_t)ks); memcpy(e.value,value,(size_t)vs); return e;
}
void entry_destructor(struct Entry *e) { if(e){free(e->key);free(e->value);free(e);} }
