#ifndef _BITMASK_UTILS_H_
#define _BITMASK_UTILS_H_
#include <stdint.h>

//Range: [0-32). Setting bit 32 does nothing.
//Recommended to use Enum values when setting bits
typedef uint32_t Bitmask;

/*!*****************************************************************************
\brief
   Sets the values given as arguments.

   Example usage: CreateBitmask(2, FLAG_0, FLAG_1)

\param[in] numOfArgs
  Number of arguments passed in after this.
  UBD if this number is more than the number of flags passed in.

\param[in] ...
  Place as many arguments as needed. Each arg should be a number (eg. from enum) to add to the mask.

\return
  A bitmask with the given flags set
*******************************************************************************/
Bitmask CreateBitmask(int numOfArgs, ...);

/*!*****************************************************************************
\brief
   Sets the flag to positive.

   Example usage: SetFlagAtPos(&myMask, FLAG_1) -> BitmaskContainsFlag(myMask, FLAG_1) returns true.

\param[out] mask
  The bitmask to update

\param[in] flag
  The position(flag) to set to 1.

\return
  Copy of the input mask
*******************************************************************************/
Bitmask SetFlagAtPos(Bitmask* mask, int flag);

/*!*****************************************************************************
\brief
   Turns off the flag (set to 0).

   Example Usage:
   BitmaskContainsFlag(mask, FLAG_1) -> true
   ResetFlagAtPos(&mask, FLAG_1) -> Turn off FLAG_1
   BitmaskContainsFlag(mask, FLAG_1) -> false

\param[out] mask
  The bitmask to update

\param[in] flag
  The flag to turn off

\return
  Copy of the modified mask.
*******************************************************************************/
Bitmask ResetFlagAtPos(Bitmask* mask, int flag);

/*!*****************************************************************************
\brief
   Flips/inverses the flag

\param[out] mask
  The bitmask to update

\param[int] flag
  The flag to flip

\return
  Copy of the modified mask.
*******************************************************************************/

Bitmask FlipBitAtPos(Bitmask* mask, int flag);

/*!*****************************************************************************
\brief
   Checks whether the bitmask has the flag set(1)

\param[out] mask
  The bitmask to update

\param[int] flag
  The flag to check for.

\return
  True if the mask contains the flag.
*******************************************************************************/
bool BitmaskContainsFlag(Bitmask mask, int flag);

/*!*****************************************************************************
\brief
   Compares if 2 bitmasks are identical.

   Example Usage:
   mask1 -> contains FLAG_1, FLAG_2
   mask2 -> contains FLAG_0, FLAG_1
   mask3 -> contains FLAG_1, FLAG_2

   CompareBitmasks(mask1, mask2) -> false
   CompareBitmasks(mask2, mask1) -> true

\param[in] a
  Mask 1

\param[out] a
  Mask 2

\return
  True if they are identical. False otherwise.
*******************************************************************************/
bool CompareBitmasks(Bitmask a, Bitmask b);

#endif // !_BITMASK_UTILS_H_
