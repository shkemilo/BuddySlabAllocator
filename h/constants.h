#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

#define NULL 0

#define TRUE 1
#define FALSE 0

#define BLOCK_SIZE (4096)

#define CACHE_L1_LINE_SIZE (64)

#define BUFFER_MIN_ORDER (5)
#define BUFFER_MAX_ORDER (18)

typedef enum SlabCategory
{
	SlabFull = 0,
	SlabPartial,
	SlabFree,
	SlabCount
} ESlabCategory;

// nothing in this code is safe, please don't remind me microsoft :)
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#endif // !_CONSTANTS_H_