#ifndef _MANAGER_H_
#define _MANAGER_H_

typedef struct kmem_buddy_s kmem_buddy_t;

typedef struct kmem_cache_s kmem_cache_t;

typedef struct kmem_manager_s
{
	void* memStart;
	unsigned blockCount;

	kmem_buddy_t* buddy;

	kmem_cache_t* allCaches;

	kmem_cache_t* cacheCache;

	kmem_cache_t** bufferCaches;
} kmem_manager_t;

void manager_init(void* space, unsigned block_count);

kmem_manager_t* manager_getInstance();

kmem_cache_t* manager_findBufferCache(kmem_manager_t* manager, void* objp);

void manager_cleanCacheCache(kmem_manager_t* manager);

#endif // !_MANAGER_H_