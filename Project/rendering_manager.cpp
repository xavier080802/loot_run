#include "rendering_manager.h"
#include "./Helpers/render_utils.h"
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

int RenderingManager::GetAnimFPS()
{
	return animationFPS;
}
