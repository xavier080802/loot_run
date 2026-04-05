#ifndef PAUSE_H_
#define PAUSE_H_

#include "AEEngine.h"

/// <summary>
/// Self-contained pause menu (Resume / Settings / Main Menu).
///
/// Usage in GameState:
///
///   Update():
///     if (AEInputCheckTriggered(AEVK_M) || AEInputCheckTriggered(AEVK_ESCAPE))
///         { Pause::Toggle(); return; }
///     if (Pause::Update()) return;   // freeze all game logic while paused
///
///   Draw() -- at the very end, after all game rendering:
///     Pause::Draw();
/// </summary>
namespace Pause
{
    /// Open the pause menu.
    void Open();

    /// Close the pause menu. Also force-closes Settings if it was open.
    void Close();

    /// Toggle open/closed.
    void Toggle();

    /// True while the pause menu is visible.
    bool IsOpen();

    /// Zeroes dt while the pause menu is open so all game systems freeze naturally.
    /// Also handles button input.
    /// Call at the top of GameState::Update() before any other game logic:
    ///     Pause::Update(dt);
    ///     if (Pause::IsOpen()) return;
    void Update(double& dt);

    /// Render the pause overlay on top of the game world.
    /// Does nothing when IsOpen() is false.
    void Draw();
}

#endif // PAUSE_H_

