#ifndef _SETTINGS_H_
#define _SETTINGS_H_
#include "AEEngine.h"

/// <summary>
/// Self-contained settings popup (Music / UI / SFX volume).
///
/// Usage from any state:
///
///   Update():
///     if (Settings::Update(scale, audioGroup, clickSound, hoverSound)) return;
///
///   Draw():
///     // draw your state content first, then:
///     Settings::Draw(font, bigFont, scale);
///
/// The caller owns all font and audio handles -- Settings never loads or
/// frees them, so there is no double-load and no lifetime issue.
/// </summary>
namespace Settings
{
    /// Open the popup. Call on Settings button click.
    void Open();

    /// Close the popup. Called internally by Close/ESC, but exposed so
    /// a pause menu can force-close it on unpause.
    void Close();

    /// True while the popup is visible.
    bool IsOpen();

    /// Volume accessors (range 0-10; multiply by 0.1f for 0.0-1.0 scale).
    int GetBGMVolume();
    int GetUIVolume();
    int GetSFXVolume();

    /// Handle popup input for one frame.
    /// Returns true if the popup is open and consumed input this frame --
    /// the caller should skip all other input processing when true.
    ///
    /// @param scale        The calling state's current UI scale factor.
    /// @param audioGroup   Audio group used for button feedback sounds.
    /// @param clickSound   Sound played on button click.
    /// @param hoverSound   Sound played when cursor enters a button.
    bool Update(float scale,
                AEAudioGroup audioGroup,
                AEAudio      clickSound,
                AEAudio      hoverSound);

    /// Render the popup overlay.
    /// Call at the very end of Draw() so the popup sits on top of everything.
    /// Does nothing when IsOpen() is false.
    ///
    /// @param font     Regular-size font (same one the calling state uses).
    /// @param bigFont  Large font used for panel title.
    /// @param scale    The calling state's current UI scale factor.
    void Draw(s8 font, s8 bigFont, float scale);
}

#endif // _SETTINGS_H_
