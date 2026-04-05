#ifndef RENDERDATA_H_
#define RENDERDATA_H_

#include "AEEngine.h"
#include "../Helpers/ColorUtils.h"
#include "RenderingManager.h"
#include <vector>

class RenderData
{
public:
	std::vector<AEGfxTexture*> texList{};
	virtual AEGfxTexture* GetTexture() const;
	virtual AEGfxTexture* SetActiveTexture(int index);
	void AddTexture(const char* texturePath);
	void ReplaceTexture(const char* texturePath, int index);
	virtual AEVec2 GetTexOffset();
	virtual AEGfxVertexList* GetMesh();
	Color tint{};
	u8 alpha{ 255 };
	MESH_SHAPE meshShape{ MESH_SHAPE::MESH_SQUARE };

	virtual void Init(MESH_SHAPE shape);
	virtual void Free();

protected:
	int currTex{};
	AEGfxVertexList* mesh{};
};

#endif // RENDERDATA_H_

