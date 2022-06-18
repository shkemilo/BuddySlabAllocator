#include "slab_impl.h"

#include "manager.h"
#include "cache_impl.h"

#include "utils.h"
#include "constants.h"
#include "error.h"

kmem_slab_t* slab_malloc(kmem_cache_t* owner, unsigned blockCount)
{
	if (!owner)
	{
		HandleError(ErrorInvalidArguments);
		return NULL;
	}

	kmem_manager_t* manager = manager_getInstance();
	if (!manager)
	{
		HandleError(ErrorNoMetadata);
		return NULL;
	}

	void* slabMem = buddy_malloc(manager->buddy, blockCount);
	if (slabMem == NULL)
	{
		owner->errorType = ErrorBuddyOutOfMemmory;
		return NULL;
	}

	kmem_slab_t* slab = (kmem_slab_t*)slabMem;

	slab->freeObjects = (int*)((char*)slabMem + sizeof(kmem_slab_t));
	slab->nextFreeObject = 0;
	slab->owner = owner;

	slab->objectSpace = (void*)((char*)slabMem + sizeof(kmem_slab_t) + sizeof(unsigned) * owner->objectCount + cache_getSlabOffset(owner));
	for (unsigned i = 0; i < owner->objectCount; ++i)
	{
		slab->freeObjects[i] = i + 1;
	}
	slab->freeObjects[owner->objectCount - 1] = -1;

	slab->usedCount = 0;

	owner->allocatedCount += owner->objectCount;

	kmem_slab_t* freeSlabs = owner->slabs[SlabFree];
	if (freeSlabs)
	{
		slab->next = freeSlabs;
		owner->slabs[SlabFree] = slab;
	}
	else
	{
		slab->next = NULL;
		owner->slabs[SlabFree] = slab;
	}

	return slab;
}

void slab_move(kmem_slab_t* slab, ESlabCategory from, ESlabCategory to)
{
	kmem_cache_t* owner = slab->owner;
	kmem_slab_t* sourceHead = owner->slabs[from];

	if (slab == sourceHead)
	{
		owner->slabs[from] = sourceHead->next;
	}
	else
	{
		kmem_slab_t* current = sourceHead;
		for (; current->next != slab; current = current->next);
		current->next = slab->next;
	}

	slab->next = owner->slabs[to];
	owner->slabs[to] = slab;
}

void slab_expand(kmem_slab_t* slab, ESlabCategory slabIn)
{
	if (slabIn == SlabFull)
	{
		return;
	}

	if (slabIn == SlabFree)
	{
		if (slab->usedCount != slab->owner->objectCount)
		{
			slab_move(slab, slabIn, SlabPartial);
		}
		else
		{
			slab_move(slab, slabIn, SlabFull);
		}
	}
	else
	{
		if (slab->usedCount == slab->owner->objectCount)
		{
			slab_move(slab, slabIn, SlabFull);
		}
	}
}

void slab_shrink(kmem_slab_t* slab, ESlabCategory slabIn)
{
	if (slabIn == SlabFree)
	{
		return;
	}

	if (slabIn == SlabFull)
	{
		if (slab->usedCount != 0)
		{
			slab_move(slab, slabIn, SlabPartial);
		}
		else
		{
			slab_move(slab, slabIn, SlabFree);
		}
	}
	else
	{
		if (slab->usedCount == 0)
		{
			slab_move(slab, slabIn, SlabFree);
		}
	}
}

unsigned slab_getNextFreeObject(kmem_slab_t* slab)
{
	unsigned objIndex = slab->nextFreeObject;
	slab->nextFreeObject = slab->freeObjects[slab->nextFreeObject];

	return objIndex;
}

char slab_containts(kmem_slab_t* slab, void* objp)
{
	return (slab->objectSpace <= objp) && ((char*)objp < ((char*)slab->objectSpace + (slab->owner->objectCount * slab->owner->objectSize)));
}
