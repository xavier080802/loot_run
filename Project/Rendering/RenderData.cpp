#include "RenderData.h"

void RenderData::Init(MESH_SHAPE shape)
{
	RenderingManager* rm = RenderingManager::GetInstance();
	mesh = rm->GetMesh(shape);
	meshShape = shape;
}

void RenderData::Free()
{
}

AEGfxTexture* RenderData::GetTexture() const
{
    return !texList.size() ? nullptr : texList.at(currTex);
}

AEGfxTexture* RenderData::SetActiveTexture(int index)
{
    if (index >= texList.size())
        return nullptr;

	currTex = index;
    return texList.at(currTex);
}

void RenderData::AddTexture(const char* texturePath)
{
	texList.push_back(RenderingManager::GetInstance()->LoadTexture(texturePath));
}

void RenderData::ReplaceTexture(const char* texturePath, int index)
{
	if (index < 0) return;
	if (index >= texList.size()) {
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
