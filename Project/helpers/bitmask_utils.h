#ifndef _BITMASK_UTILS_H_
#define _BITMASK_UTILS_H_
#include <stdint.h>

//Range: [0-32). Setting bit 32 does nothing.
//Recommended to use Enum values when setting bits
typedef uint32_t Bitmask;

/// <summary>
/// Sets the values given as arguments.
/// 
/// Eg. CreateBitmask(2, FLAG_0, FLAG_1)
/// </summary>
/// <param name="numOfArgs">
/// Number of arguments passed in after this one, indicating the number of flags to set.
/// UBD if this number is more than the number of flags passed in.
/// </param>
/// <param name="...">Flags to pass in</param>
/// <returns>The created bitmask</returns>
Bitmask CreateBitmask(int numOfArgs, ...);

/// <summary>
/// Sets the flag positive
/// </summary>
/// <param name="mask">[Out] modifies the mask</param>
/// <param name="flag">The flag to set to 1</param>
/// <returns>Copy of the modified mask</returns>
Bitmask SetFlagAtPos(Bitmask* mask, int flag);

/// <summary>
/// Turns off the flag (set to 0)
/// </summary>
/// <param name="mask">[Out] modifies this mask</param>
/// <param name="flag">The flag to turn off</param>
/// <returns>Copy of the modified mask</returns>
Bitmask ResetFlagAtPos(Bitmask* mask, int flag);

/// <summary>
/// Inverses the flag
/// </summary>
/// <param name="mask">[Out] modifies this mask</param>
/// <param name="flag">The flag to flip</param>
/// <returns>Copy of the modified mask</returns>
Bitmask FlipBitAtPos(Bitmask* mask, int flag);

/// <summary>
/// Checks whether the bitmask has the flag
/// </summary>
/// <param name="mask">Mask to check</param>
/// <param name="flag">Flag to check for</param>
/// <returns>0 if does not contain. Any +ve num if it does contain</returns>
bool BitmaskContainsFlag(Bitmask mask, int flag);

/// <summary>
/// Compares if 2 bitmasks are identical
/// </summary>
/// <returns>1 or 0</returns>
int CompareBitmasks(Bitmask a, Bitmask b);

#endif // !_BITMASK_UTILS_H_
