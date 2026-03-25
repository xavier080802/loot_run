#include "RenderingManager.h"
#include "./Helpers/RenderUtils.h"
#include "UIConfig.h"
#include <iostream>

void RenderingManager::Init()
{
	//Font
	fontId = AEGfxCreateFont(PRIMARY_FONT_PATH, fontSize);
	f32 tmp{};
	AEGfxGetPrintSize(fontId, "P", 1.f, &tmp, &fontHeight); //Get height of some char

	//Create meshes
	for (int i{}; i < MESH_SHAPE::SHAPE_NUM; i++) {
		switch (i)
		{
		case MESH_SHAPE::MESH_SQUARE:
			meshList[i] = CreateSquareMesh(1, 1, 0xFFFFFFFF);
			break;
		case MESH_SHAPE::MESH_CIRCLE:
			meshList[i] = CreateCircleMesh(0.5f, CreateColor(255, 255, 255, 255), 60);
			break;
		case MESH_SHAPE::MESH_BORDER:
			//4-point mesh for a border
			AEGfxMeshStart();
			AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0); AEGfxVertexAdd(0.5f, -0.5f, 0xFFFFFFFF, 0, 0);
			AEGfxVertexAdd(0.5f, 0.5f, 0xFFFFFFFF, 0, 0); AEGfxVertexAdd(-0.5f, 0.5f, 0xFFFFFFFF, 0, 0);
			AEGfxVertexAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0, 0); 
			meshList[i] = AEGfxMeshEnd();
			break;
		case MESH_SHAPE::MESH_SQUARE_ANIM: //Need spritesheet row/cols, left to AnimationData
			break;
		default:
			std::cout << "Mesh shape Init not implemented: " << i << std::endl;
			meshList[i] = nullptr;
			break;
		}
	}
}

AEGfxVertexList* RenderingManager::GetMesh(MESH_SHAPE shape)
{
	return meshList[shape];
}

AEGfxTexture* RenderingManager::LoadTexture(const char* path)
{
	if (textureMap.find(path) != textureMap.end()) {
		return textureMap.find(path)->second;
	}
	AEGfxTexture* tex = AEGfxTextureLoad(path);
	textureMap.insert(std::pair < std::string, AEGfxTexture*>(path, tex));
	return tex;
}

AEGfxTexture* RenderingManager::LoadTexture(std::string path)
{
	return LoadTexture(path.c_str());
}

f32 RenderingManager::GetFontHeight(s8 id) const
{
	f32 tmp{}, out{};
	AEGfxGetPrintSize(id, "P", 1.f, &tmp, &out); //Get height of some char
	return out;
}

RenderingManager::~RenderingManager()
{
	for (std::pair <std ::string, AEGfxTexture*> pair : textureMap) {
		if (pair.second == nullptr) continue;
		AEGfxTextureUnload(pair.second);
		pair.second = nullptr;
	}
	for (AEGfxVertexList* v : meshList) {
		if (!v) continue;
		AEGfxMeshFree(v);
		v = nullptr;
	}
	textureMap.clear();
	AEGfxDestroyFont(fontId);
}
