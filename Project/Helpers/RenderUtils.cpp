#include "RenderUtils.h"
#include "MatrixUtils.h"
#include "Vec2Utils.h"
#include "CoordUtils.h"
#include "../Camera.h"
#include "../RenderingManager.h"
#include <sstream>

// ------NOTE ABOUT ROTATION-------
// Normally, CW is +ve. So in matrices and calculations, we use +ve for CW.
// But rendering an object with a +ve angle shows it ACW
// Thus, when using inverse rot, the value is theta instead of -theta 
// And for the normal theta, when passing to a rendering function, should be passed as -theta.

TextOriginPos ParseTextAlignment(std::string const& str)
{
	if (str == "TEXT_LOWER_RIGHT") return TEXT_LOWER_RIGHT;
	if (str == "TEXT_UPPER_LEFT") return TEXT_UPPER_LEFT;
	if (str == "TEXT_UPPER_RIGHT") return TEXT_UPPER_RIGHT;
	if (str == "TEXT_MIDDLE") return TEXT_MIDDLE;
	if (str == "TEXT_MIDDLE_LEFT") return TEXT_MIDDLE_LEFT;
	if (str == "TEXT_MIDDLE_RIGHT") return TEXT_MIDDLE_RIGHT;
	if (str == "TEXT_LOWER_MIDDLE") return TEXT_LOWER_MIDDLE;
	if (str == "TEXT_UPPER_MIDDLE") return TEXT_UPPER_MIDDLE;
	return TEXT_LOWER_LEFT;
}

AEGfxVertexList* CreateSquareMesh(f32 width, f32 height, u32 color)
{
	AEGfxMeshStart();

	AEGfxTriAdd(
		-width/2.f, -height/2.f, color, 0.0f, 1.0f,
		width/2.f, -height/2.f, color, 1.0f, 1.0f,
		-width / 2.f, height/2.f, color, 0.0f, 0.0f);

	AEGfxTriAdd(
		width / 2.f, -height / 2.f, color, 1.0f, 1.0f,
		width / 2.f, height / 2.f, color, 1.0f, 0.0f,
		-width / 2.f, height / 2.f, color, 0.0f, 0.0f);

	return AEGfxMeshEnd();
}

AEGfxVertexList* CreateSquareAnimMesh(f32 width, f32 height, u32 color, int rows, int cols)
{
	AEGfxMeshStart();

	AEGfxTriAdd(
		-width / 2.f, -height / 2.f, color, 0.0f, 1.0f/(float)rows,
		width / 2.f, -height / 2.f, color, 1.0f/(float)cols, 1.0f/(float)rows,
		-width / 2.f, height / 2.f, color, 0.0f, 0.0f);

	AEGfxTriAdd(
		width / 2.f, -height / 2.f, color, 1.0f/(float)cols, 1.0f/(float)rows,
		width / 2.f, height / 2.f, color, 1.0f/(float)cols, 0.0f,
		-width / 2.f, height / 2.f, color, 0.0f, 0.0f);

	return AEGfxMeshEnd();
}

AEGfxVertexList* CreateCircleMesh(f32 radius, Color col, unsigned int slices)
{
	AEGfxMeshStart();
	f32 theta = 360.f / slices;
	u32 uCol = ColToHex(col);

	for (unsigned int i = 1; i <= slices*2; i++) {
		f32 x0 = radius * AECosDeg((i - 1) * theta),
			y0 = radius * AESinDeg((i - 1) * theta),
			x2 = radius * AECosDeg(i * theta),
			y2 = radius * AESinDeg(i * theta);
		//UV: -y + 0.5 to map vertex coord to uv coord.
		AEGfxTriAdd(
			x0, y0, uCol, x0 + 0.5f, -y0 + 0.5f,
			0, 0, uCol, 0.5f, 0.5f,
			x2, y2, uCol, x2 + 0.5f, -y2 + 0.5f);
	}

	return AEGfxMeshEnd();
}

void DrawMesh(AEMtx33 transform, AEGfxVertexList* mesh, AEGfxTexture* tex, float alpha)
{
	DrawTintedMesh(transform, mesh, tex, CreateColor(0, 0, 0, 0), alpha);
}

void DrawTintedMesh(AEMtx33 transform, AEGfxVertexList* mesh, AEGfxTexture* tex, Color tint, float alpha)
{
	DrawMeshWithTexOffset(transform, mesh, tex, tint, alpha, VecZero());
}

void DrawMeshWithTexOffset(AEMtx33 transform, AEGfxVertexList* mesh, AEGfxTexture* tex, Color tint, float alpha, AEVec2 texOffset)
{
	if (!mesh) { return; }

	AEGfxSetRenderMode(tex ? AE_GFX_RM_TEXTURE : AE_GFX_RM_COLOR);

	// Set the the color to multiply to white, so that the sprite can 
	// display the full range of colors (default is black).
	//AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

	// Tint
	AEGfxSetColorToMultiply(tint.r/255.f, tint.g/255.f, tint.b/255.f, tint.a/255.f);

	// Set blend mode to AE_GFX_BM_BLEND, which will allow transparency.
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(alpha / 255.f);

	// Tell Alpha Engine to use the texture stored in pTex
	if (tex) AEGfxTextureSet(tex, AEClamp(texOffset.x, 0.f, 1.f), AEClamp(texOffset.y, 0.f, 1.f));

	// Tell Alpha Engine to use the matrix in 'transform' to apply onto all 
	// the vertices of the mesh that we are about to choose to draw in the next line.
	AEGfxSetTransform(transform.m);

	// Tell Alpha Engine to draw the mesh with the above settings.
	AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
}

void DrawBox(AEVec2 center, f32 width, f32 height, f32 thickness, Color col)
{
	u32 hexCol = ColToHex(col);
	AEGfxVertexList* vtx[4] = {
		CreateSquareMesh(width + thickness*2.f, thickness, hexCol), //top
		CreateSquareMesh(thickness, height + thickness*2.f, hexCol), //left
		CreateSquareMesh(width + thickness*2.f, thickness, hexCol), //bottom
		CreateSquareMesh(thickness, height + thickness*2.f, hexCol) // right
	};
	DrawMesh(GetTransformMtx(ToVec2(center.x, center.y - height / 2.f - thickness/2.f), 0, ToVec2(1.f, 1.f)),
		vtx[0], NULL, col.a);
	DrawMesh(GetTransformMtx(ToVec2(center.x - width/2.f - thickness / 2.f, center.y), 0, ToVec2(1.f, 1.f)),
		vtx[1], NULL, col.a);
	DrawMesh(GetTransformMtx(ToVec2(center.x, center.y + height / 2.f + thickness / 2.f), 0, ToVec2(1.f, 1.f)),
		vtx[2], NULL, col.a);
	DrawMesh(GetTransformMtx(ToVec2(center.x + width / 2.f + thickness / 2.f, center.y), 0, ToVec2(1.f, 1.f)),
		vtx[3], NULL, col.a);

	for (int i = 0; i < 4; i++) {
		AEGfxMeshFree(vtx[i]);
	}
}

AEVec2 GetTextAlignPosNorm(s8 const& font, std::string const& text, AEVec2 pos, f32 descFontSize, TextOriginPos alignment)
{
	f32 w, h;
	AEGfxGetPrintSize(font, text.c_str(), descFontSize, &w, &h);
	return GetTextAlignPosNorm(font, text, pos, AEVec2{w,h}, alignment);
}

AEVec2 GetTextAlignPosNorm(s8 const& font, std::string const& text, AEVec2 pos, AEVec2 size, TextOriginPos alignment)
{
	AEVec2 normPos = WorldToNorm(pos);

	//Offset text based on chosen anchor
	switch (alignment)
	{
	case TEXT_MIDDLE:
		normPos.x -= size.x *0.5f;
		normPos.y -= size.y *0.5f;
		break;
	case TEXT_MIDDLE_LEFT:
		normPos.y -= size.y *0.5f;
		break;
	case TEXT_MIDDLE_RIGHT:
		normPos.x -= size.x;
		normPos.y -= size.y *0.5f;
		break;
	case TEXT_LOWER_MIDDLE:
		normPos.x -= size.x *0.5f;
		break;
	case TEXT_UPPER_MIDDLE:
		normPos.x -= size.x *0.5f;
		normPos.y -= size.y;
		break;
	case TEXT_UPPER_LEFT:
		normPos.y -= size.y;
		break;
	case TEXT_UPPER_RIGHT:
		normPos.x -= size.x;
		normPos.y -= size.y;
		break;
	case TEXT_LOWER_RIGHT:
		normPos.x -= size.x;
		break;
	case TEXT_LOWER_LEFT:
	default:
		break;
	}
	return normPos;
}

void DrawAEText(s8 const& font, const char* text, AEVec2 pos, f32 size, Color const& col, TextOriginPos alignment, bool isHUD)
{
	if (!isHUD) {
		AEVec2 scale = ToVec2(size, size);
		float rot = 0;
		GetObjViewFromCamera(&pos, &rot, &scale);
		size = scale.x;
	}
	AEVec2 normPos = GetTextAlignPosNorm(font, text, pos, size, alignment);

	AEGfxPrint(font, text,
		normPos.x, normPos.y,
		size,
		col.r/255.f, col.g/255.f, col.b/255.f, col.a/255.f);
}

void DrawAEText(s8 const& font, std::string const& text, AEVec2 pos, f32 descFontSize, f32 lineSpace, Color const& col, TextOriginPos alignment, bool isHUD)
{
	//Split string based on newlines
	size_t i{}; //End of substr
	size_t start{}; //Start of substr
	size_t num{}; //Num of newlines
	do {
		i = text.find_first_of('\n', start);
		//No newline, or last line might not have a newline char, check if theres more chars
		if (i == std::string::npos) {
			if (start < text.size())
				i = text.size();
			else break; //Is at end of str, exit
		}

		std::string sub{ text.substr(start, i - start) };
		f32 w, h;
		AEGfxGetPrintSize(font, sub.c_str(), descFontSize, &w, &h);
		//Draw line below previous line (if any)
		DrawAEText(font, sub.c_str(), { pos.x, pos.y - ((h+lineSpace)*AEGfxGetWinMaxY()) * num }, descFontSize, col, alignment, isHUD);

		num++;
		start = i+1;
	} while (i != std::string::npos);
}

void GetAETextSize(s8 const& font, std::string const& text, f32 descFontSize, f32& width, f32& height, f32 lineSpace)
{
	//Split string based on newlines
	size_t i{}; //End of substr
	size_t start{}; //Start of substr
	do {
		i = text.find_first_of('\n', start);
		//No newline, or last line might not have a newline char, check if theres more chars
		if (i == std::string::npos) {
			if (start < text.size())
				i = text.size();
			else break; //Is at end of str, exit
		}

		std::string sub{ text.substr(start, i - start) };
		f32 w, h;
		AEGfxGetPrintSize(font, sub.c_str(), descFontSize, &w, &h);
		width = max(w, width); //Line with longest width
		//If h is 0, might be "\n\n", add prev height. If start = 0, this is first line, dont add lineSpace
		height += (h == 0 ? height : h) + (start ? lineSpace : 0); 

		start = i + 1;
	} while (i != std::string::npos);
}

TextboxOriginPos ParseTextboxAlignment(std::string const& strval)
{
	if (strval == "BOTTOM") return TextboxOriginPos::BOTTOM;
	else return TextboxOriginPos::TOP;
}

AEVec2 DrawAETextbox(s8 const& font, std::string const& text, AEVec2 pos, f32 boxWidth, f32 descFontSize, f32 lineSpace, Color const& col,
	TextOriginPos textAlignment, TextboxOriginPos boxAlignment, TextboxBgCfg const& bgCfg, bool isHUD)
{
	//Split string based on newlines and word wrapping to fit box width
	size_t i{}; //End of substr
	size_t start{}; //Start of substr
	std::ostringstream wrapped;
	do {
		i = text.find_first_of('\n', start);
		//No newline, or last line might not have a newline char, check if theres more chars
		if (i == std::string::npos) {
			if (start < text.size())
				i = text.size();
			else break; //Is at end of str, exit
		}
		std::string sub{ text.substr(start, i - start) };

		//Wordwrap this line
		std::istringstream words{ sub };
		std::string word;
		if (words >> word) {
			//First word
			wrapped << word;
			f32 length, h;
			AEGfxGetPrintSize(font, word.c_str(), descFontSize, &length, &h);
			length *= AEGfxGetWinMaxX();
			size_t space_left = boxWidth - length;

			//Wrap the rest of the text
			while (words >> word) {
				AEGfxGetPrintSize(font, word.c_str(), descFontSize, &length, &h);
				length *= AEGfxGetWinMaxX();
				if (space_left < length + 1) {
					wrapped << '\n' << word;
					space_left = boxWidth - length;
				}
				else {
					wrapped << ' ' << word;
					space_left -= length + 1;
				}
			}
		}
		wrapped << '\n'; //Add back original new line

		start = i + 1;
	} while (i != std::string::npos);

	//Offset to keep box on screen
	static const f32 fontH{RenderingManager::GetInstance()->GetFontHeight() * descFontSize};
	AEVec2 acBoxSize{};
	GetAETextSize(font, wrapped.str(), descFontSize, acBoxSize.x, acBoxSize.y, lineSpace);

	//Norm-pos of text around pos (origin of textbox) - effectively normalized amount of offset from pos
	AEVec2 norm = GetTextAlignPosNorm(font, wrapped.str(), {}, {acBoxSize.x, fontH}, textAlignment);
	if (norm.y == 0) norm.y -= fontH; //Og anchor was bottom of first line (default so y=0), move textbox down so top of text is at anchor.
	else if (norm.y == -fontH) norm.y = 0; //Og anchor is top, which is textbox anchor, so cancel the change
	
	acBoxSize.x *= AEGfxGetWinMaxX(); //Norm to world
	acBoxSize.y *= AEGfxGetWinMaxY(); //Norm to world
	switch (boxAlignment) //Move so that the vertical anchor is at the designated location
	{
	case TextboxOriginPos::BOTTOM: //Bottom of box is to be at the original pos
		pos.y += acBoxSize.y;
		break;
	case TextboxOriginPos::TOP:
		pos.y += norm.y * AEGfxGetWinMaxY();
		break;
	default:
		break;
	}
	//Checking the edges of the box (pos + norm_to_world) after text alignment
	AEVec2 edge{ pos.x+norm.x * AEGfxGetWinMaxX(), pos.y - norm.y * AEGfxGetWinMaxY()}; //Top-left
	//Check right side
	if (edge.x + acBoxSize.x > AEGfxGetWinMaxX()) { //right side is cut off on the right
		pos.x -= (edge.x + acBoxSize.x) - AEGfxGetWinMaxX(); //Shift till right-side is at the right border
	}
	//Check left side
	if (edge.x < AEGfxGetWinMinX()) {
		pos.x += abs(AEGfxGetWinMinX() - edge.x); //Shift right
	}
	//Check bottom side
	if (edge.y - acBoxSize.y < AEGfxGetWinMinY()) {
		pos.y += abs(AEGfxGetWinMinY() - (edge.y - acBoxSize.y));
	}
	//Check top side
	if (edge.y > AEGfxGetWinMaxY()) {
		pos.y -= edge.y - AEGfxGetWinMaxY();
	}
	if (bgCfg.haveBg) { //Draw background box
		//Reset edge calc to render bg correctly
		edge = { pos.x + norm.x * AEGfxGetWinMaxX(), pos.y - norm.y * AEGfxGetWinMaxY() };
		DrawTintedMesh(GetTransformMtx({ edge.x + acBoxSize.x * 0.5f, edge.y - acBoxSize.y * 0.5f }, 0, acBoxSize + NormToWorld(bgCfg.padding)),
			bgCfg.mesh, bgCfg.texture, bgCfg.col, bgCfg.alpha);
	}
	//Write text
	DrawAEText(font, wrapped.str(), pos, descFontSize, lineSpace, col, textAlignment, isHUD);

	return pos;
}

void SetObjViewFromOrigin(AEVec2* pos, f32* rot, AEVec2* _scale, AEVec2 originPos, f32 originRot, AEVec2 originScale)
{
	*_scale = MultVec2(*_scale, originScale);
	AEVec2 ogPos = *pos;
	//Offsets due to rotation and zoom (displacement from camera)
	pos->x = originPos.x + (ogPos.x - originPos.x) * AECosDeg(originRot) * originScale.x - (ogPos.y - originPos.y) * AESinDeg(originRot) * originScale.y;
	pos->y = originPos.y + (ogPos.x - originPos.x) * AESinDeg(originRot) * originScale.x + (ogPos.y - originPos.y) * AECosDeg(originRot) * originScale.y;
	//Translation due to cam movement
	*pos -= originPos;
	//Offset rotation 
	*rot += originRot;
}
