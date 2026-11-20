#include "Dictionary.h"
#include <stdlib.h>
#include <string.h>
static int compare_entry_ptrs(void*a,void*b){struct Entry*x=*(struct Entry**)a,*y=*(struct Entry**)b;return x->compare(x->key,y->key);}
static void insert_dict(struct Dictionary*d,void*k,unsigned long ks,void*v,unsigned long vs){if(!d||!k||!v||!ks||!vs)return;struct Entry probe={k,NULL,d->key_compare};struct Entry*probe_p=&probe;struct Entry**found=d->binary_search_tree.search(&d->binary_search_tree,&probe_p);if(found&&*found){void*nv=malloc(vs);if(!nv)return;memcpy(nv,v,vs);free((*found)->value);(*found)->value=nv;return;}struct Entry*e=malloc(sizeof(*e));if(!e)return;*e=entry_constructor(k,ks,v,vs);if(!e->key||!e->value){entry_destructor(e);return;}e->compare=d->key_compare;d->binary_search_tree.insert(&d->binary_search_tree,&e,sizeof(e));d->keys.insert(&d->keys,d->keys.length,k,ks);}
static void *search_dict(struct Dictionary*d,void*k,unsigned long ks){(void)ks;if(!d||!k)return NULL;struct Entry probe={k,NULL,d->key_compare};struct Entry*p=&probe;struct Entry**found=d->binary_search_tree.search(&d->binary_search_tree,&p);return found&&*found?(*found)->value:NULL;}
static void destroy_entries(struct Node*n){if(!n)return;destroy_entries(n->previous);destroy_entries(n->next);struct Entry*e=*(struct Entry**)n->data;entry_destructor(e);free(n->data);free(n);}
struct Dictionary dictionary_constructor(int(*cmp)(void*,void*)){struct Dictionary d={0};d.key_compare=cmp;d.binary_search_tree=binary_search_tree_constructor(compare_entry_ptrs);d.keys=linked_list_constructor();d.insert=insert_dict;d.search=search_dict;return d;}
void dictionary_destructor(struct Dictionary*d){if(!d)return;linked_list_destructor(&d->keys);destroy_entries(d->binary_search_tree.head);d->binary_search_tree.head=NULL;}
int compare_string_keys(void*a,void*b){return strcmp((const char*)a,(const char*)b);}
