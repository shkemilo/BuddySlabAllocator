#include "manager.h"

#include "buddy.h"
#include "cache_impl.h"
#include "slab_impl.h"

#include <string.h>

#include "constants.h"
#include "utils.h"

kmem_manager_t* manager = NULL;

void manager_init(void* space, unsigned blockCount)
{
	// can't init the manager more then once...
	if (manager != NULL)
	{
		return;
	}

	if (!space || blockCount < 2)
	{
		HandleError(ErrorInvalidArguments);
		return;
	}

	manager = (kmem_manager_t*)space;
	manager->memStart = space;
	manager->blockCount = blockCount;

	manager->buddy = buddy_init((void*)((char*)space + BLOCK_SIZE), blockCount - 1);

	manager->allCaches = NULL;
	manager->cacheCache = cache_create((void*)((char*)space + sizeof(kmem_manager_t)), "kmem_cacheCache\0", sizeof(kmem_cache_t), NULL, NULL);
	manager->bufferCaches = (kmem_cache_t**)((char*)space + sizeof(kmem_manager_t) + sizeof(kmem_cache_t));

	kmem_slab_t* slab = slab_malloc(manager->cacheCache, 1);
}

kmem_manager_t* manager_getInstance()
{
	return manager;
}

kmem_cache_t* manager_findBufferCache(kmem_manager_t* manager, void* objp)
{
	if (manager == NULL || objp == NULL)
	{
		HandleError(ErrorInvalidArguments);
		return NULL;
	}

	for (unsigned i = BUFFER_MIN_ORDER; i < BUFFER_MAX_ORDER; ++i)
	{
		kmem_cache_t* bufferCache = manager->bufferCaches[BUFFER_ORDER_TO_INDEX(i)];

		kmem_slab_t* objSlab = cache_findSlab(bufferCache, SlabPartial, objp);
		if (objSlab == NULL)
		{
			objSlab = cache_findSlab(bufferCache, SlabFull, objp);
		}

		if (objSlab == NULL)
		{
			// the object is not a buffer of order i
			continue;
		}

		return bufferCache;
	}

	// the object is not a buffer
	return NULL;
}

void manager_cleanCacheCache(kmem_manager_t* manager)
{
	if (manager == NULL)
	{
		HandleError(ErrorInvalidArguments);
		return;
	}

	WaitForSingleObject(manager->cacheCache->mutex, INFINITE);
	kmem_slab_t* cacheCacheFree = manager->cacheCache->slabs[SlabFree];
	if (cacheCacheFree != NULL)
	{
		// always leave one free slab in cacheCache
		cacheCacheFree = cacheCacheFree->next;
		while (cacheCacheFree != NULL)
		{
			kmem_slab_t* cacheSlab = cacheCacheFree;
			cacheCacheFree = cacheCacheFree->next;

			cacheSlab->next = NULL;
			buddy_free(manager->buddy, (void*)cacheSlab, manager->cacheCache->blockCount);
			manager->cacheCache->allocatedCount -= manager->cacheCache->objectCount;
		}
	}
	ReleaseMutex(manager->cacheCache->mutex);
}