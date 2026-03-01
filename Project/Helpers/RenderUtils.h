//---------------------------------------------------------
// author:    Edna Sim
//
// Copyright © 2025 DigiPen, All rights reserved.
//---------------------------------------------------------
#ifndef _RENDER_UTILS_H_
#define _RENDER_UTILS_H_
#include "AEEngine.h"
#include "ColorUtils.h"
#include <string>

//Reference used for word wrap algorithm
//https://rosettacode.org/wiki/Word_wrap#C++

/*
	Note about textbox/multiline:

	AEGfxGetPrintSize acts kinda weird, making some lines overlap each other due
	to the height being different between lines.

	Therefore, to maintain consistency in the textbox, multiline functions use RenderingManager's generalized
	font height.
*/

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

//Parse Enum name (String) to the enum value
TextOriginPos ParseTextAlignment(std::string const& str);

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
/// <param name="tint">Ensure that alpha is > 0. Multiplicative coloring</param>
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

//Get normalized position of text based on text alignment
//Pass in normalized size to skip the AEGfxGetPrintSize call.
AEVec2 GetTextAlignPosNorm(s8 const& font, std::string const& text, AEVec2 pos, AEVec2 normSize, TextOriginPos alignment);

// Get normalized position of text based on text alignment
AEVec2 GetTextAlignPosNorm(s8 const& font, std::string const& text, AEVec2 pos, f32 fontSize, TextOriginPos alignment);

/// <summary>
/// Render text.
/// Newlines are ignored, use the other overload.
/// </summary>
/// <param name="text">Text to write</param>
/// <param name="pos">Position of the text anchor</param>
/// <param name="size">Scale of the text based on the initialization size.</param>
/// <param name="alignment">Where the text is relative to the anchor. Anchor is at pos</param>
/// <param name="isHUD">If false, rendered in the world, based on the camera. Text always reorients to the camera</param>
void DrawAEText(s8 const& font, const char* text, AEVec2 pos, f32 size, Color const& col, TextOriginPos alignment, bool isHUD=true);

/// <summary>
/// Render text with newlines.
/// Subsequent lines are drawn below the first line.
/// Therefore, the origin(pos) of the "textbox" is at the top.
/// </summary>
/// <param name="text">Text to write.</param>
/// <param name="pos">Position of the first line of text</param>
/// <param name="fontSize">Scale of the text based on the initialization size.</param>
/// <param name="lineSpace">Scale of extra distance between lines based on font initialization size</param>
/// <param name="alignment">Where the text is relative to the anchor. Anchor is at pos, with the y offset for each line</param>
/// <param name="isHUD">If false, rendered in the world, based on the camera. Text always reorients to the camera</param>
void DrawAEText(s8 const& font, std::string const& text, AEVec2 pos, f32 fontSize, f32 lineSpace, Color const& col, TextOriginPos alignment, bool isHUD = true);

/// <summary>
/// Gets the NORMALIZED width and height of the "textbox".
/// Note that width and height will not be reset to 0 before calculation.
/// Assumes w and h are initialized.
/// </summary>
/// <param name="width">[out] The width of the text</param>
/// <param name="height">[out] The height of the text, including newlines</param>
/// <param name="fontSize">Scale of the text based on the initialization size.</param>
/// <param name="lineSpace">Scale of extra distance between lines based on font initialization size</param>
void GetAEMultilineTextSize(s8 const& font, std::string const& text, f32 fontSize, f32& width, f32& height, f32 lineSpace = 0);

//Config for the background of a textbox
struct TextboxBgCfg {
	bool haveBg{ false };
	AEVec2 padding{};
	Color col{};
	float alpha{};
	AEGfxVertexList* mesh{};
	AEGfxTexture* texture{};

	TextboxBgCfg() : haveBg{ false } {}

	TextboxBgCfg(AEVec2 _padding, Color _col, float _alpha, AEGfxVertexList* _mesh, AEGfxTexture* _tex = nullptr)
		: haveBg{ true }, padding{ _padding }, col{ _col }, alpha{ _alpha }, mesh{ _mesh }, texture{ _tex } {
	}
};

//Which part of the box should be at the anchor
enum class TextboxOriginPos {
	TOP, //Default
	BOTTOM //Making the textbox appear above pos
};
TextboxOriginPos ParseTextboxAlignment(std::string const& strval);

/// <summary>
/// Writes text like it's in a textbox, with word wrapping.
/// Tries its best to keep the whole box on screen (prioritising the top left corner)
/// </summary>
/// <param name="pos">Position of the TOP anchor of the textbox</param>
/// <param name="boxWidth">Width the box should be. Sentences that overflow will attempt to wrap</param>
/// <param name="fontSize">Scale of the text based on initialization size. (1.f is normal size)</param>
/// <param name="lineSpace">NORMALIZED distance between the bottom of one line and the top of the next</param>
/// <param name="textAlignment">Alignment of text relative to the anchor of the textbox. H-alignment dictates the X-anchor of the box</param>
/// <param name="boxAlignment">Location of the anchor of the textbox.</param>
/// <param name="bgCfg">Config for the rendering of the box. Use default ctor if you dont want to render the box</param>
/// <returns>Modified world pos of the box to follow alignment</returns>
AEVec2 DrawAETextbox(s8 const& font, std::string const& text, AEVec2 pos, f32 boxWidth, f32 fontSize, f32 lineSpace, Color const& col,
	TextOriginPos textAlignment, TextboxOriginPos boxAlignment,
	TextboxBgCfg const& bgCfg = TextboxBgCfg{}, bool isHUD = true);

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
