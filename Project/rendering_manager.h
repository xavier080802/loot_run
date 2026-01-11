#ifndef _RENDERING_MANAGER_H_
#define _RENDERING_MANAGER_H_
#include "./DesignPatterns/singleton.h"
#include "AEEngine.h"

enum MESH_SHAPE {
	MESH_SQUARE,
	MESH_CIRCLE,
	MESH_SQUARE_ANIM,

	SHAPE_NUM, // Last
};

class RenderingManager : public Singleton<RenderingManager>
{
	friend Singleton<RenderingManager>;
public:
	void Init();
	AEGfxVertexList* GetMesh(MESH_SHAPE shape);

private:
	AEGfxVertexList* meshList[SHAPE_NUM]{};
};

#endif // !_RENDERING_MANAGER_H_

