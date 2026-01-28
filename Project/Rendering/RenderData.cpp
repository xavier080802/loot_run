#include "RenderData.h"

void RenderData::Init(MESH_SHAPE shape)
{
	RenderingManager* rm = RenderingManager::GetInstance();
	mesh = rm->GetMesh(shape);
	meshShape = shape;
}

void RenderData::Free()
{
	delete texList;
	texList = nullptr;
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
		printf("ERROR: Failed to realloc for texture in AddTexture. | %s\n", texturePath);
		//Free prev
		free(prev);
		return;
	}
	texList[texCount - 1] = RenderingManager::GetInstance()->LoadTexture(texturePath);
}

void RenderData::ReplaceTexture(const char* texturePath, int index)
{
	if (index < 0) return;
	if (index >= texCount) {
		return AddTexture(texturePath);
	}
	texList[index] = RenderingManager::GetInstance()->LoadTexture(texturePath);
}

AEVec2 RenderData::GetTexOffset()
{
	return {};
}

AEGfxVertexList* RenderData::GetMesh()
{
	return mesh;
}
