#include "render_data.h"

void RenderData::Init(MESH_SHAPE shape)
{
	RenderingManager* rm = RenderingManager::GetInstance();
	mesh = rm->GetMesh(shape);
	meshShape = shape;
}

void RenderData::Free()
{
	for (int i = 0; i < texCount; i++) {
		if (!texList[i]) continue;
		AEGfxTextureUnload(texList[i]);
	}
}

AEGfxTexture* RenderData::GetTexture() const
{
    return !texCount ? nullptr : texList[currTex];
}

AEGfxTexture* RenderData::SetActiveTexture(int index)
{
    if (index >= texCount)
        return nullptr;

    return texList[currTex = index];
}

void RenderData::AddTexture(const char* texturePath)
{
	AEGfxTexture** prev = texList;
	texList = (AEGfxTexture**)realloc(texList, sizeof(AEGfxTexture*) * ++texCount);
	if (!texList) {
		printf("ERROR: Failed to realloc for render this texture in AddTexture. | %s\n", texturePath);
		//Free prev
		for (int i = 0; i < texCount; i++) {
			if (!prev[i]) continue;
			AEGfxTextureUnload(prev[i]);
			prev[i] = NULL;
		}
		free(prev);
		return;
	}
	texList[texCount - 1] = AEGfxTextureLoad(texturePath);
}

AEVec2 RenderData::GetTexOffset()
{
	return {};
}

AEGfxVertexList* RenderData::GetMesh()
{
	return mesh;
}
