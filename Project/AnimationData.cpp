#include "AnimationData.h"
#include "./Helpers/RenderUtils.h"

AnimationData* AnimationData::InitAnimation(unsigned rows, unsigned cols)
{
	spriteSheetRows = rows;
	spriteSheetCols = cols;
	startFrame = 0;
	endFrame = rows * cols - 1;
	//Create mesh
	if (mesh) {
		AEGfxMeshFree(mesh);
	}
	mesh = CreateSquareAnimMesh(1, 1, 0xFFFFFFFF, spriteSheetRows, spriteSheetCols);
	return this;
}

AnimationData* AnimationData::LoopAnim(bool loop)
{
	isLooping = loop;
	return this;
}

AnimationData* AnimationData::SetAnimRange(unsigned start, unsigned end)
{
	if (start < end) return this;
	startFrame = start;
	endFrame = end;
	//Apply now if needed
	if (animCurrFrame > endFrame || animCurrFrame < startFrame) {
		animCurrFrame = startFrame;
	}
	return this;
}

AnimationData* AnimationData::SetFrameCallback(void(*callback)(unsigned,bool))
{
	animFrameCallback = callback;
	return this;
}

void AnimationData::Init(MESH_SHAPE shape)
{
	manager = RenderingManager::GetInstance();
	RenderData::Init(shape);
	if (shape != MESH_SQUARE_ANIM) {
		isPaused = true;
	}
}

void AnimationData::Free()
{
	if (mesh) {
		AEGfxMeshFree(mesh);
		mesh = nullptr;
	}
	RenderData::Free();
}

AEVec2 AnimationData::GetTexOffset()
{
	if (meshShape != MESH_SQUARE_ANIM) return {};

	int currAnimRow = animCurrFrame / (spriteSheetCols);
	int currAnimCol = animCurrFrame % (spriteSheetCols);
	return { (float)currAnimCol / (float)spriteSheetCols, (float)currAnimRow / (float)spriteSheetRows };
}

void AnimationData::UpdateAnimation(double dt)
{
	if (isPaused || (!isLooping && isAnimCycleComplete)) {
		return;
	}
	//Cycle through frames
	animTimer += dt;
	if (animTimer >= 1.0 / (double)manager->GetAnimFPS()) {
		//Next frame
		animTimer = 0.0;
		animCurrFrame++;
		if (animCurrFrame > endFrame) { //Last frame done
			animCurrFrame = startFrame;
		}

		if (animFrameCallback) {
			animFrameCallback(animCurrFrame, animCurrFrame == endFrame);
		}
	}
	isAnimCycleComplete = animCurrFrame == endFrame;
}

void AnimationData::PauseAnim(bool pause)
{
	isPaused = pause;
}

void AnimationData::SetFrame(unsigned frameIndex)
{
	//Allow going past startFrame and endFrame as long as it remains within the spritesheet.
	animCurrFrame = (int)AEClamp((float)frameIndex, 0, (float)(spriteSheetRows * spriteSheetCols - 1));
	animTimer = 0.0;
}
