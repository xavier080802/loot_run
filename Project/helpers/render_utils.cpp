#include "render_utils.h"
#include "matrix_utils.h"
#include "vec2_utils.h"
#include "coord_utils.h"
#include "../camera.h"

// ------NOTE ABOUT ROTATION-------
// Normally, CW is +ve. So in matrices and calculations, we use +ve for CW.
// But rendering an object with a +ve angle shows it ACW
// Thus, when using inverse rot, the value is theta instead of -theta 
// And for the normal theta, when passing to a rendering function, should be passed as -theta.

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
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

	// Tint
	AEGfxSetColorToAdd(tint.r, tint.g, tint.b, tint.a);

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

void DrawAEText(s8& font, const char* text, AEVec2 pos, f32 size, Color col, TextOriginPos alignment, int isHUD)
{
	if (!isHUD) {
		AEVec2 scale = ToVec2(size, size);
		float rot = 0;
		GetObjViewFromCamera(&pos, &rot, &scale);
		size = scale.x;
	}
	f32 w, h;
	AEGfxGetPrintSize(font, text, size, &w, &h);
	AEVec2 normPos = WorldToNorm(pos);
	
	//Offset text based on chosen anchor
	switch (alignment)
	{
	case TEXT_MIDDLE:
		normPos.x -= w / 2.f;
		normPos.y -= h / 2.f;
		break;
	case TEXT_MIDDLE_LEFT:
		normPos.y -= h / 2.f;
		break;
	case TEXT_MIDDLE_RIGHT:
		normPos.x -= w;
		normPos.y -= h / 2.f;
		break;
	case TEXT_LOWER_MIDDLE:
		normPos.x -= w / 2.f;
		break;
	case TEXT_UPPER_MIDDLE:
		normPos.x -= w / 2.f;
		normPos.y -= h;
		break;
	case TEXT_UPPER_LEFT:
		normPos.y -= h;
		break;
	case TEXT_UPPER_RIGHT:
		normPos.x -= w;
		normPos.y -= h;
		break;
	case TEXT_LOWER_RIGHT:
		normPos.x -= w;
		break;
	case TEXT_LOWER_LEFT:
	default:
		break;
	}

	AEGfxPrint(font, text,
		normPos.x, normPos.y,
		size,
		col.r/255.f, col.g/255.f, col.b/255.f, col.a/255.f);
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
