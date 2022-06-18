#include "cache_impl.h"

#include "manager.h"

#include <string.h>

#include "slab_impl.h"
#include "constants.h"
#include "utils.h"

kmem_cache_t* cache_create(void* space, const char* name, unsigned size, void(*ctor)(void*), void(*dtor)(void*))
{
	kmem_manager_t* manager = manager_getInstance();
	kmem_cache_t* newCache = (kmem_cache_t*)space;

	// init new cache
	strcpy(newCache->name, name);

	newCache->slabs[SlabFull] = newCache->slabs[SlabPartial] = newCache->slabs[SlabFree] = NULL;

	newCache->mutex = CreateMutex(NULL, FALSE, NULL);

	newCache->ctor = ctor;
	newCache->dtor = dtor;
	newCache->errorType = ErrorNoError;

	if (manager->allCaches)
	{
		newCache->next = manager->allCaches;
		manager->allCaches = newCache;
	}
	else
	{
		newCache->next = NULL;
		manager->allCaches = newCache;
	}

	// find new cache order

	unsigned order = 0;
	unsigned objects = 0;
	while (1)
	{
		unsigned i = 1 << order;
		objects = (i * BLOCK_SIZE - sizeof(kmem_slab_t)) / (sizeof(int) + size);
		if (objects == 0)
		{
			order++;
			continue;
		}
		break;
	}

	newCache->objectSize = size;
	newCache->blockCount = pow2(order);
	newCache->objectCount = objects;
	newCache->allocatedCount = 0;

	newCache->maxColour = ((newCache->blockCount * BLOCK_SIZE - sizeof(kmem_slab_t) - sizeof(unsigned) * newCache->objectCount) % newCache->objectSize) / CACHE_L1_LINE_SIZE;
	newCache->nextColour = newCache->maxColour != 0;

	return newCache;
}

void cache_freeSlabs(kmem_cache_t* cache)
{
	if (cache == NULL)
	{
		HandleError(ErrorInvalidArguments);
		return;
	}

	kmem_manager_t* manager = manager_getInstance();
	if (manager == NULL)
	{
		HandleError(ErrorNoMetadata);
		return;
	}

	for (ESlabCategory category = SlabFull; category < SlabCount; ++category)
	{
		kmem_slab_t* slabHead = cache->slabs[category];
		while (slabHead != NULL)
		{
			void* slabData = (void*)slabHead;
			slabHead = slabHead->next;
			buddy_free(manager->buddy, slabData, cache->blockCount);
		}
	}
}

unsigned cache_getSlabOffset(kmem_cache_t* cache)
{
	unsigned offset = cache->nextColour * CACHE_L1_LINE_SIZE;
	if (cache->maxColour != 0)
	{
		cache->nextColour = (cache->nextColour + 1) % cache->maxColour;
	}

	return offset;
}

kmem_slab_t* cache_findSlab(kmem_cache_t* cache, ESlabCategory category, void* objp)
{
	if (cache == NULL || objp == NULL)
	{
		HandleError(ErrorInvalidArguments);
		return NULL;
	}

	// we wont find an object in a empty slab
	if (category == SlabFree)
	{
		return NULL;
	}

	kmem_slab_t* slab = cache->slabs[category];
	while (slab != NULL)
	{
		if (slab_containts(slab, objp))
		{
			return slab;
		}
		slab = slab->next;
	}

	return NULL;
}
