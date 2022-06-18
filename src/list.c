#include "list.h"

#include "error.h"
#include "constants.h"

void list_put(list_t* head, unsigned index, void* data)
{
	if (!head)
	{
		HandleError(ErrorInvalidArguments);
		return;
	}

	node_t* node = (node_t*)data;
	if (head[index])
	{
		head[index]->previous = node;
	}

	node->next = head[index];
	head[index] = node;
	node->previous = NULL;
}

void* list_get(list_t* head, unsigned index)
{
	if (!head)
	{
		HandleError(ErrorInvalidArguments);
		return NULL;
	}

	node_t* node = head[index];
	if (node)
	{
		node_t* next = node->next;
		if (next)
		{
			next->previous = NULL;
		}

		head[index] = node->next;
		node->previous = 0;
		node->next = 0;
	}

	return (void*)node;
}

void list_remove(list_t* head, node_t* node)
{
	if (!head)
	{
		HandleError(ErrorInvalidArguments);
		return;
	}

	if (*head == node)
	{
		*head = node->next;
	}

	if (node->next)
	{
		node->next->previous = node->previous;
	}

	if (node->previous)
	{
		node->previous->next = node->next;
	}
}
