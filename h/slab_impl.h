#ifndef _SLAB_IMPL_H
#define _SLAB_IMPL_H

#include "buddy.h"
#include "constants.h"
#include "error.h"

typedef struct kmem_cache_s kmem_cache_t;

typedef struct kmem_slab_s
{
	void* objectSpace;

	int nextFreeObject;
	int* freeObjects;

	unsigned usedCount;

	struct kmem_slab_s* next;

	kmem_cache_t* owner;
} kmem_slab_t;

kmem_slab_t* slab_malloc(kmem_cache_t* owner, unsigned blockCount);

void slab_move(kmem_slab_t* slab, ESlabCategory from, ESlabCategory to);

void slab_expand(kmem_slab_t* slab, ESlabCategory slabIn);

void slab_shrink(kmem_slab_t* slab, ESlabCategory slabIn);

unsigned slab_getNextFreeObject(kmem_slab_t* slab);

char slab_containts(kmem_slab_t* slab, void* objp);

#endif // !_SLAB_IMPL_H
