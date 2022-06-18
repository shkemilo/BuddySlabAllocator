#ifndef _UTILS_H_
#define _UTILS_H_

#include "constants.h"

#define BUFFER_ORDER_TO_INDEX(i) (i - BUFFER_MIN_ORDER)

#define pow2(n) (1 << n)

inline unsigned log2(unsigned value)
{
	unsigned calculated = 0;
	while (value > 0)
	{
		value >>= 1;
		calculated++;
	}

	return calculated;
}

#endif // !_UTILS_H_