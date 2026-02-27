#ifndef _RENDERING_MANAGER_H_
#define _RENDERING_MANAGER_H_
#include "./DesignPatterns/singleton.h"
#include "AEEngine.h"
#include <map>
#include <string>

enum MESH_SHAPE {
	MESH_SQUARE,
	MESH_CIRCLE,
	MESH_SQUARE_ANIM,
	MESH_BORDER, //Draw with AE_GFX_MDM_LINES_STRIP

	SHAPE_NUM, // Last
};

/// <summary>
/// Caches common meshes, and all textures.
/// 
/// Objects should not load and unload textures by themselves.
/// Loading the same texture through AE will alloc a new instance.
/// Loading from memory risks another obj unloading that texture
/// when it is freed.
/// </summary>
class RenderingManager : public Singleton<RenderingManager>
{
	friend Singleton<RenderingManager>;
public:
	void Init();

	//Do not Free/delete this mesh outside of RenderingManager script
	AEGfxVertexList* GetMesh(MESH_SHAPE shape);
	//Loads and caches a texture.
	//Multiple objs using the same tex will not duplicate mem
	//and won't break the other objs when deleting texture.
	AEGfxTexture* LoadTexture(const char* path);
	s8 GetFont() const { return fontId; }
	int GetFontSize() const { return fontSize; }
	f32 GetFontHeight() const { return fontHeight; }
	int GetAnimFPS();

private:
	const int fontSize{ 72 };
	AEGfxVertexList* meshList[SHAPE_NUM]{};
	//<filepath, texture>
	std::map<std::string, AEGfxTexture*>textureMap;
	s8 fontId;
	f32 fontHeight;
	int animationFPS{ 5 };

	~RenderingManager();
};

#endif // !_RENDERING_MANAGER_H_

