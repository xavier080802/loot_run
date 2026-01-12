#include "bitmask_utils.h"
#include <stdarg.h>

Bitmask CreateBitmask(int numOfArgs, ...)
{
	Bitmask ret = 0;
	va_list args;
	va_start(args, numOfArgs);
	for (int i = 0; i < numOfArgs; i++) {
		int val = va_arg(args, int);
		SetFlagAtPos(&ret, val);
	}
	va_end(args);
	return ret;
}

Bitmask SetFlagAtPos(Bitmask* mask, int flag)
{
	*mask |= 1 << flag;
	return *mask;
}

Bitmask ResetFlagAtPos(Bitmask* mask, int flag)
{
	*mask &= ~(1 << flag);
	return *mask;
}

Bitmask FlipBitAtPos(Bitmask* mask, int flag)
{
	*mask ^= 1 << flag;
	return *mask;
}

bool BitmaskContainsFlag(Bitmask mask, int flag)
{
	//Returns the value of the bit as 2^pos. Pos 0 is 1
	return mask & (1 << flag);
}

int CompareBitmasks(Bitmask a, Bitmask b)
{
	return (a & b) == a;
}
