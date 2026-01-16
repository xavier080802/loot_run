#include "RenderingManager.h"
#include "./Helpers/RenderUtils.h"
#include <iostream>

void RenderingManager::Init()
{
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

int RenderingManager::GetAnimFPS()
{
	return animationFPS;
}

RenderingManager::~RenderingManager()
{
	for (std::pair <std ::string, AEGfxTexture*> pair : textureMap) {
		AEGfxTextureUnload(pair.second);
		pair.second = nullptr;
	}
	textureMap.clear();
}
