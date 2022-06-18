#include "slab.h"

#include "windows.h"

#include <stdio.h>
#include <string.h>

#include "manager.h"
#include "buddy.h"
#include "cache_impl.h"
#include "slab_impl.h"

#include "constants.h"
#include "utils.h"

void kmem_init(void* space, int block_num)
{
	manager_init(space, block_num);

	kmem_manager_t* manager = manager_getInstance();

	for (unsigned i = BUFFER_MIN_ORDER; i < BUFFER_MAX_ORDER; ++i)
	{
		char bufferName[MAX_CACHE_NAME_LENGTH];
		sprintf(bufferName, "buffer size-%d\0", i);
		kmem_cache_t* bufferCache = kmem_cache_create(bufferName, pow2(i), NULL, NULL);
		manager->bufferCaches[BUFFER_ORDER_TO_INDEX(i)] = bufferCache;
	}
}

kmem_cache_t* kmem_cache_create(const char* name, size_t size, void(*ctor)(void*), void(*dtor)(void*))
{
	if (name == NULL || *name == '\0')
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

	kmem_cache_t* cacheCache = manager->cacheCache;
	WaitForSingleObject(cacheCache->mutex, INFINITE);

	// check if cache allready exists
	kmem_slab_t* slab;

	for(kmem_cache_t* cache = manager->allCaches; cache != NULL; cache = cache->next)
	{
		if ((strcmp(cache->name, name) == 0) && cache->objectSize == size)
		{
			ReleaseMutex(cacheCache->mutex);
			return cache;
		}
	}

	// it doesnt

	slab = cacheCache->slabs[SlabPartial];
	if (!slab)
	{
		slab = cacheCache->slabs[SlabFree];
	}

	if (!slab)
	{
		slab = slab_malloc(cacheCache, 1);
	}

	kmem_cache_t* list = (kmem_cache_t*)slab->objectSpace;

	unsigned index = slab_getNextFreeObject(slab);
	++(slab->usedCount);

	ESlabCategory slabIn = SlabFree;
	if (slab != cacheCache->slabs[SlabFree])
	{
		slabIn = SlabPartial;
	}

	slab_expand(slab, slabIn);

	kmem_cache_t* newCache = cache_create(&list[index], name, size, ctor, dtor);
	ReleaseMutex(cacheCache->mutex);
	return newCache;
}

int kmem_cache_shrink(kmem_cache_t* cachep)
{
	if (!cachep)
	{
		HandleError(ErrorInvalidArguments);
		return 0;
	}

	kmem_manager_t* manager = manager_getInstance();
	if (!manager)
	{
		HandleError(ErrorNoMetadata);
		return 0;
	}
	WaitForSingleObject(cachep->mutex, INFINITE);

	cachep->errorType = ErrorNoError;

	if (cachep->slabs[SlabFree] == NULL)
	{
		ReleaseMutex(cachep->mutex);
		return 0;
	}

	unsigned blocksFreed = 0;
	while (cachep->slabs[SlabFree])
	{
		kmem_slab_t* slab = cachep->slabs[SlabFree];
		buddy_free(manager->buddy, slab, cachep->blockCount);
		blocksFreed += cachep->blockCount;

		cachep->slabs[SlabFree] = cachep->slabs[SlabFree]->next;
	}

	ReleaseMutex(cachep->mutex);
	return blocksFreed;
}

void* kmem_cache_alloc(kmem_cache_t* cachep)
{
	if (cachep == NULL || *cachep->name == '\0')
	{
		HandleError(ErrorInvalidArguments);
		return NULL;
	}
	WaitForSingleObject(cachep->mutex, INFINITE);

	cachep->errorType = ErrorNoError;

	kmem_slab_t* slab = cachep->slabs[SlabPartial];
	if (slab == NULL)
	{
		slab = cachep->slabs[SlabFree];
	}
	if (slab == NULL)
	{
		slab = slab_malloc(cachep, cachep->blockCount);

		// move slab to partial as a object will be allocated
		slab_move(slab, SlabFree, SlabPartial);
	}

	void* objp = (void*)((char*)slab->objectSpace + slab_getNextFreeObject(slab) * cachep->objectSize);
	++(slab->usedCount);

	ESlabCategory slabIn = SlabFree;
	if (slab != cachep->slabs[slabIn])
	{
		slabIn = SlabPartial;
	}

	slab_expand(slab, slabIn);

	if (cachep->ctor)
	{
		cachep->ctor(objp);
	}

	ReleaseMutex(cachep->mutex);
	return objp;
}

void kmem_cache_free(kmem_cache_t* cachep, void* objp)
{
	if (cachep == NULL || *cachep->name == '\0' || objp == NULL)
	{
		HandleError(ErrorInvalidArguments);
		return;
	}
	WaitForSingleObject(cachep->mutex, INFINITE);

	cachep->errorType = ErrorNoError;

	ESlabCategory slabIn = SlabFull;
	kmem_slab_t* slab = cache_findSlab(cachep, slabIn, objp);

	if (slab == NULL)
	{
		slabIn = SlabPartial;
		slab = cache_findSlab(cachep, slabIn, objp);
	}

	if (slab == NULL)
	{
		cachep->errorType = ErrorNoSlabForObject;
		ReleaseMutex(cachep->mutex);
		return;
	}

	--slab->usedCount;
	int index = ((char*)objp - (char*)slab->objectSpace) / cachep->objectSize;
	if (objp != (void*)((char*)slab->objectSpace + index * cachep->objectSize))
	{
		cachep->errorType = ErrorInvalidObject;
		ReleaseMutex(cachep->mutex);
		return;
	}

	slab->freeObjects[index] = slab->nextFreeObject;
	slab->nextFreeObject = index;

	if (cachep->dtor != NULL)
	{
		cachep->dtor(objp);
	}
	if (cachep->ctor != NULL)
	{
		cachep->ctor(objp);
	}

	slab_shrink(slab, slabIn);
	ReleaseMutex(cachep->mutex);
}

void* kmalloc(size_t size)
{
	kmem_manager_t* manager = manager_getInstance();
	if (manager == NULL)
	{
		HandleError(ErrorNoMetadata);
		return NULL;
	}

	unsigned bufferOrder = log2(size);
	if (bufferOrder < BUFFER_MIN_ORDER || bufferOrder >= BUFFER_MAX_ORDER)
	{
		HandleError(ErrorInvalidArguments);
		return NULL;
	}

	return kmem_cache_alloc(manager->bufferCaches[BUFFER_ORDER_TO_INDEX(bufferOrder)]);
}

void kfree(const void* objp)
{
	if (objp == NULL)
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

	kmem_cache_t* bufferCache = manager_findBufferCache(manager, (void*)objp);
	if (bufferCache == NULL)
	{
		HandleError(ErrorInvalidObject);
		return;
	}

	kmem_cache_free(bufferCache, (void*)objp);
}

void kmem_cache_destroy(kmem_cache_t* cachep)
{
	if (cachep == NULL || *cachep->name == '\0')
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
	WaitForSingleObject(cachep->mutex, INFINITE);

	cachep->errorType = ErrorNoError;

	if (cachep == manager->cacheCache)
	{
		cachep->errorType = ErrorDeleteSystemCache;
		ReleaseMutex(cachep->mutex);
		return;
	}

	kmem_cache_t* previous = NULL;
	kmem_cache_t* current = manager->allCaches;
	while (current != cachep)
	{
		previous = current;
		current = current->next;
	}

	if (current == NULL)
	{
		cachep->errorType = ErrorCacheNotExsist;
		ReleaseMutex(cachep->mutex);
		return;
	}

	if (previous == NULL)
	{
		manager->allCaches = manager->allCaches->next;
	}
	else
	{
		previous->next = current->next;
	}
	current->next = NULL;


	WaitForSingleObject(manager->cacheCache->mutex, INFINITE);
	ESlabCategory slabIn = SlabFull;
	kmem_slab_t* slab = cache_findSlab(manager->cacheCache, slabIn, cachep);
	if (slab == NULL)
	{
		slabIn = SlabPartial;
		slab = cache_findSlab(manager->cacheCache, slabIn, cachep);
	}
	ReleaseMutex(manager->cacheCache->mutex);

	if (slab == NULL)
	{
		cachep->errorType = ErrorCacheNotExsist;
		ReleaseMutex(cachep->mutex);
		return;
	}

	// reset cache fields
	--slab->usedCount;
	int index = cachep - (kmem_cache_t*)slab->objectSpace;
	slab->freeObjects[index] = slab->nextFreeObject;
	slab->nextFreeObject = index;
	*cachep->name = '\0';

	cache_freeSlabs(cachep);

	slab_shrink(slab, slabIn);

	manager_cleanCacheCache(manager);
	ReleaseMutex(cachep->mutex);
}

void kmem_cache_info(kmem_cache_t* cachep)
{
	if (cachep == NULL)
	{
		HandleError(ErrorInvalidArguments);
		return;
	}
	WaitForSingleObject(cachep->mutex, INFINITE);

	unsigned cacheSize = 0;
	unsigned slabCount = 0;
	double occupancy = 0;
	for (ESlabCategory category = SlabFull; category < SlabCount; ++category)
	{
		for (kmem_slab_t* slab = cachep->slabs[category]; slab != NULL; slab = slab->next)
		{
			++slabCount;
			occupancy += ((1.0 * slab->usedCount) / cachep->objectCount);
		}
	}
	cacheSize = slabCount * cachep->blockCount;
	occupancy = (occupancy / slabCount) * 100;

	printf("\n==============================\n"
		"Cache: %s\n"
		"Object size: %u\n"
		"Cache size: %u\n"
		"Slab count: %u\n"
		"Slab Object count: %u\n"
		"Cache occupancy: %.2lf\n"
		"==============================\n\n", 
		cachep->name, cachep->objectSize, cacheSize, slabCount, cachep->objectCount, occupancy);

	ReleaseMutex(cachep->mutex);
}

int kmem_cache_error(kmem_cache_t* cachep)
{
	if (cachep == NULL)
	{
		HandleError(ErrorInvalidArguments);
		return ErrorInvalidArguments;
	}
	WaitForSingleObject(cachep->mutex, INFINITE);
	EErrorType error = cachep->errorType;
	HandleError(error);

	ReleaseMutex(cachep->mutex);
	return error;
}
