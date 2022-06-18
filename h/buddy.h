#ifndef _BUDDY_H_
#define _BUDDY_H_

#include "windows.h"

#include "list.h"

typedef void* data;

typedef struct kmem_buddy_s
{
	data startAddres;

	HANDLE mutex;

	unsigned indexCount;
	list_t* freeList;
} kmem_buddy_t;

kmem_buddy_t* buddy_init(data start, unsigned blocks);

data buddy_malloc(kmem_buddy_t* buddy, unsigned blocks);

void buddy_free(kmem_buddy_t* buddy, data block, unsigned blocks);

void buddy_destruct(kmem_buddy_t* buddy, unsigned blocks);

#endif // !_BUDDY_H_