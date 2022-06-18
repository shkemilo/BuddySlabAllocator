#ifndef _CACHE_IMPL_H_
#define _CACHE_IMPL_H_

#include "windows.h"

#include "constants.h"
#include "error.h"

#define MAX_CACHE_NAME_LENGTH 32

typedef struct kmem_slab_s kmem_slab_t;

typedef struct kmem_cache_s
{
	char name[MAX_CACHE_NAME_LENGTH];

	kmem_slab_t* slabs[SlabCount];

	unsigned blockCount;

	unsigned objectSize;
	unsigned objectCount;
	unsigned allocatedCount;

	unsigned maxColour;
	unsigned nextColour;

	EErrorType errorType;

	HANDLE mutex;

	void (*ctor)(void*);
	void (*dtor)(void*);

	struct kmem_cahce_s* next;
} kmem_cache_t;

kmem_cache_t* cache_create(void* space, const char* name, unsigned size, void(*ctor)(void*), void(*dtor)(void*));

void cache_freeSlabs(kmem_cache_t* cache);

unsigned cache_getSlabOffset(kmem_cache_t* cache);

kmem_slab_t* cache_findSlab(kmem_cache_t* cache, ESlabCategory category, void* objp);

#endif // !_MANAGER_H_