#ifndef _RENDER_DATA_H_
#define _RENDER_DATA_H_
#include "AEEngine.h"
#include "./Helpers/color_utils.h"
#include "rendering_manager.h"

//TODO: anim data. Override Init to create anim mesh. and override Free to release anim mesh
class RenderData
{
public:
	AEGfxTexture** texList;
	virtual AEGfxTexture* GetTexture() const;
	virtual AEGfxTexture* SetActiveTexture(int index);
	void AddTexture(const char* texturePath);
	virtual AEVec2 GetTexOffset();
	virtual AEGfxVertexList* GetMesh();
	Color tint;
	u8 alpha;
	MESH_SHAPE meshShape{ MESH_SHAPE::SQUARE };

	virtual void Init(MESH_SHAPE shape);
	virtual void Free();

protected:
	int currTex{};
	int texCount{};
	AEGfxVertexList* mesh;
};

#endif // !_RENDER_DATA_H_
