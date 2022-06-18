#include "buddy.h"

#include <string.h>

#include "utils.h"
#include "constants.h"
#include "error.h"

#define RELATIVE_ADDRESS(buddy, address) (unsigned)((char*)address - (char*)buddy->startAddres)
#define BLOCK_FROM_RELATIVE_ADDRESS(address) address / BLOCK_SIZE - 1;

static inline areBuddies(kmem_buddy_t* buddy, data block1, data block2, unsigned size)
{
	unsigned blockAdress1 = RELATIVE_ADDRESS(buddy, block1);
	unsigned blockNumber1 = BLOCK_FROM_RELATIVE_ADDRESS(blockAdress1);

	unsigned blockAdress2 = RELATIVE_ADDRESS(buddy, block2);
	unsigned blockNumber2 = BLOCK_FROM_RELATIVE_ADDRESS(blockAdress2);

	return ((blockNumber1 ^ pow2(size)) == blockNumber2);
}

static void split(kmem_buddy_t* buddy, char* data, int upper, int lower)
{
	while (--upper >= lower)
	{
		list_put(buddy->freeList, upper, data + pow2(upper) * BLOCK_SIZE);
	}
}

kmem_buddy_t* buddy_init(data start, unsigned blocks)
{
	if (!start)
	{
		HandleError(ErrorInvalidArguments);
		return NULL;
	}

	kmem_buddy_t* buddy = (kmem_buddy_t*)start;

	buddy->mutex = CreateMutex(NULL, FALSE, NULL);
	buddy->startAddres = (data)((char*)start + BLOCK_SIZE); // reserve one block for buddy metadata
	buddy->indexCount = log2(blocks) - 1;
	buddy->freeList = (list_t*)((char*)start + sizeof(kmem_buddy_t));
	memset(buddy->freeList, 0, (buddy->indexCount + 1) * sizeof(list_t));

	list_put(buddy->freeList, buddy->indexCount, ((char*)buddy->startAddres + BLOCK_SIZE));

	char* addr = ((char*)buddy->startAddres + (pow2(buddy->indexCount) * BLOCK_SIZE));
	int remainingBlocks = blocks - pow2(buddy->indexCount) - 1;

	while (remainingBlocks > 0)
	{
		unsigned index = 0;
		unsigned newRemainingBlocks = remainingBlocks;
		while (newRemainingBlocks > 0)
		{
			index++;
			newRemainingBlocks >>= 1;
		}
		index--;
		list_put(buddy->freeList, index, addr);
		remainingBlocks -= pow2(index);
		addr += pow2(index) * BLOCK_SIZE;
	}

	return buddy;
}

data buddy_malloc(kmem_buddy_t* buddy, unsigned blocks)
{
	unsigned size = log2(blocks) - 1;
	if (!buddy || size > buddy->indexCount)
	{
		HandleError(ErrorInvalidArguments);
		return NULL;
	}

	WaitForSingleObject(buddy->mutex, INFINITE);

	for (unsigned i = size; i <= buddy->indexCount; ++i)
	{
		char* dataAdress = (char*)list_get(buddy->freeList, i);
		if (!dataAdress)
		{
			continue;
		}

		split(buddy, dataAdress, i, size);

		ReleaseMutex(buddy->mutex);
		return (data)dataAdress;
	}

	HandleError(ErrorBuddyOutOfMemmory);

	ReleaseMutex(buddy->mutex);
	return NULL;
}

void buddy_free(kmem_buddy_t* buddy, data block, unsigned blocks)
{
	unsigned size = log2(blocks) - 1;
	if (!buddy || !block)
	{
		HandleError(ErrorInvalidArguments);
		return;
	}
	WaitForSingleObject(buddy->freeList, INFINITE);

	char buddyFound = FALSE;
	unsigned indexToFree = size;
	for (unsigned i = size; i <= buddy->indexCount; ++i)
	{
		for (node_t* node = buddy->freeList[i]; node != NULL; node = node->next)
		{
			if (abs((char*)node - (char*)block) == (pow2(i) * BLOCK_SIZE))
			{
				if (!areBuddies(buddy, node, block, i))
				{
					continue;
				}
				buddyFound = TRUE;

				list_remove(&(buddy->freeList[i]), node);

				if (block > node)
				{
					block = (data)node;
				}
				indexToFree = i + 1;
				break;
			}
		}

		if (!buddyFound)
		{
			break;
		}
		buddyFound = FALSE;
	}

	list_put(buddy->freeList, indexToFree, block);
	ReleaseMutex(buddy->mutex);
}

void buddy_destruct(kmem_buddy_t* buddy, unsigned blocks)
{
	memset(buddy->freeList, 0, (buddy->indexCount + 1) * sizeof(list_t));
	memset(buddy, 0, sizeof(kmem_buddy_t));
}