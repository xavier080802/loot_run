#include "CoordUtils.h"
#include "../Rendering/Camera.h"

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

AEVec2 NormToWorld(AEVec2 normCoords)
{
    return AEVec2{
        normCoords.x * AEGfxGetWinMaxX(),
        normCoords.y * AEGfxGetWinMaxY()
    };
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
    AEVec2 cam = GetCameraTranslation();
    AEVec2 out = ScreenToWorld(screenCoords);
    out.x += cam.x;
    out.y += cam.y;
    //Mouse screen-to-world coords is the offset from "origin"
    AEVec2 originScale{ GetCameraZoom(), GetCameraZoom() };
    //Divide by cam zoom to negate it (Based on GetObjViewFromCamera)
    out.x = cam.x + (out.x - cam.x) / originScale.x;
    out.y = cam.y + (out.y - cam.y) / originScale.y;
    return out;
}
