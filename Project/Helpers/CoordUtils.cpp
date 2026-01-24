#include "CoordUtils.h"
#include "../Camera.h"

AEVec2 ScreenToNorm(AEVec2 screenCoords)
{
    AEVec2 out = {
        screenCoords.x / AEGfxGetWinMaxX() - 1.f,
        screenCoords.y / AEGfxGetWinMaxY() + 1.f
    };
    return out;
}

AEVec2 WorldToNorm(AEVec2 worldCoords)
{
    AEVec2 out = {
        worldCoords.x / AEGfxGetWinMaxX(),
        worldCoords.y / AEGfxGetWinMaxY()
    };
    return out;
}

AEVec2 ScreenToWorld(AEVec2 screenCoords)
{
    AEVec2 out = {
        (screenCoords.x - AEGfxGetWinMaxX()),
       -(screenCoords.y - AEGfxGetWinMaxY())
    };
    return out;
}

AEVec2 WorldToScreen(AEVec2 worldCoords)
{
    AEVec2 out = {
        worldCoords.x + AEGfxGetWinMaxX()/2.f,
        worldCoords.y + AEGfxGetWinMaxY()/2.f
    };
    return out;
}

AEVec2 ScreenToCameraWorld(AEVec2 screenCoords)
{
    AEVec2 out = ScreenToWorld(screenCoords);
    AEVec2 cam = GetCameraTranslation();
    out.x += cam.x;
    out.y += cam.y;
    return out;
}
