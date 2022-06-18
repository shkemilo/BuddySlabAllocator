#ifndef _LIST_H_
#define _LIST_H_

typedef struct node
{
	struct node* previous;
	struct node* next;
} node_t;

typedef node_t* list_t;

void list_put(list_t* head, unsigned index, void* data);

void* list_get(list_t* head, unsigned index);

void list_remove(list_t* head, node_t* node);

#endif // !_LIST_H_