//---------------------------------------------------------
// author:    Edna Sim
//
// Copyright © 2025 DigiPen, All rights reserved.
//---------------------------------------------------------
#ifndef _RENDER_UTILS_H_
#define _RENDER_UTILS_H_
#include "AEEngine.h"
#include "color_utils.h"

/// <summary>
/// Where the anchor of the text is, relative to the text.
/// Default: origin is at lower-left
/// </summary>
enum TextOriginPos {
	//Default
	TEXT_LOWER_LEFT,
	TEXT_LOWER_RIGHT,
	TEXT_UPPER_LEFT,
	TEXT_UPPER_RIGHT,
	TEXT_MIDDLE,
	TEXT_MIDDLE_LEFT,
	TEXT_MIDDLE_RIGHT,
	TEXT_LOWER_MIDDLE,
	TEXT_UPPER_MIDDLE
};

AEGfxVertexList* CreateSquareMesh(f32 width, f32 height, u32 color);

/// <summary>
/// A mesh ready for a sprite sheet of the given rows x cols.
/// </summary>
AEGfxVertexList* CreateSquareAnimMesh(f32 width, f32 height, u32 color, int rows, int cols);

//Note: radius should be 0.5f.
AEGfxVertexList* CreateCircleMesh(f32 radius, Color col, unsigned int slices);

/// <summary>
/// Draws a mesh
/// </summary>
/// <param name="transform">Translation, rotation, scale</param>
/// <param name="mesh">Shape</param>
/// <param name="tex">Texture (image)</param>
/// <param name="alpha">0 to 255</param>
void DrawMesh(AEMtx33 transform, AEGfxVertexList* mesh, AEGfxTexture* tex, float alpha);

/// <summary>
/// Draws a mesh with tint
/// </summary>
/// <param name="tint">Ensure that alpha is > 0. Additive coloring</param>
/// <param name="alpha">Transparency of the mesh</param>
void DrawTintedMesh(AEMtx33 transform, AEGfxVertexList* mesh, AEGfxTexture* tex, Color tint, float alpha);

/// <summary>
/// Draws a tinted mesh with texture offset (For animation)
/// </summary>
///<param name="tint">Ensure that alpha is > 0. Additive coloring</param>
/// <param name="alpha">Transparency of the mesh</param>
/// <param name="texOffset">each value must be 0 to 1</param>
void DrawMeshWithTexOffset(AEMtx33 transform, AEGfxVertexList* mesh, AEGfxTexture* tex, Color tint, float alpha, AEVec2 texOffset);

/// <summary>
/// Draws a hollow box (AKA outline). 
/// Thickness expands outwards, leaving the effective area within to be exactly
/// width x height.
/// </summary>
/// <param name="center">Center of the box</param>
/// <param name="width">Width of the inner box</param>
/// <param name="height">Height of the inner box</param>
/// <param name="thickness">Thickness of the box's lines</param>
/// <param name="col">Color of the box (Box is hollow)</param>
void DrawBox(AEVec2 center, f32 width, f32 height, f32 thickness, Color col);

/// <summary>
/// Render text
/// </summary>
/// <param name="text">Text to write</param>
/// <param name="pos">Position of the text anchor</param>
/// <param name="size">Scale of the text based on the initialization size.</param>
/// <param name="alignment">Where the anchor is relative to the text. Anchor is at pos</param>
/// <param name="isHUD">If false, rendered in the world, based on the camera. Text always reorients to the camera</param>
void DrawAEText(s8& font, const char* text, AEVec2 pos, f32 size, Color col, TextOriginPos alignment, int isHUD);

/// <summary>
/// Basically sets the object anchor to a position in the World. <para/>
/// 
/// Eg. World objects move, rotate and zoom based on the camera.
/// </summary>
/// <param name="pos">[Out]</param>
/// <param name="rot">[Out]</param>
/// <param name="_scale">[Out]</param>
/// <param name="originPos">Position of the anchor</param>
/// <param name="originRot">Rotation of the anchor</param>
/// <param name="originScale">Scale of the anchor</param>
void SetObjViewFromOrigin(AEVec2* pos, f32* rot, AEVec2* _scale,
	AEVec2 originPos, f32 originRot, AEVec2 originScale);

#endif // !_RENDER_UTILS_H_
