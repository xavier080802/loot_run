#ifndef _GAME_END_H_
#define _GAME_END_H_
#include "AEEngine.h"

namespace GameEnd
{
	void Show(bool won, bool isEndless, float runTime,
			  int coinsGained, int killCount);

	/// Hide and reset the overlay (called automatically by the buttons).
	void Hide();

	/// True while the overlay is visible.
	bool IsOpen();

	/// Zeroes dt while the overlay is open so all game systems freeze naturally.
	/// Also handles button input.
	void Update(double& dt);

	/// Render the overlay on top of everything.
	/// Does nothing when IsOpen() is false.
	void Draw();
}

#endif // _GAME_END_H_
