#pragma once
#include "RenderData.h"
#include "../GameObjects/GameObjectManager.h"

class AnimationData : public RenderData
{
	friend class GameObjectManager; //Allow manager access to private members.
public:
	//Creates the anim mesh
	AnimationData* InitAnimation(unsigned rows, unsigned cols);
	//Set loop flag
	AnimationData* LoopAnim(bool loop = true);
	//Change what frames the anim starts/ends on. [start, end]
	AnimationData* SetAnimRange(unsigned start, unsigned end);
	//Set the function to call when a new frame plays
	AnimationData* SetFrameCallback(void (*callback)(unsigned,bool));

	void Init(MESH_SHAPE shape = MESH_SQUARE_ANIM) override;
	void Free() override;
	AEVec2 GetTexOffset() override;

	void UpdateAnimation(double dt);
	void 
		Anim(bool pause = true);
	//Set animation to given frame and restart anim timer
	void SetFrame(unsigned frameIndex);

private:
	RenderingManager* manager{};
	unsigned spriteSheetRows{ 1 };
	unsigned spriteSheetCols{ 1 };

	//To customize the start/end frame. These are indexes
	unsigned startFrame{}, endFrame{};

	bool isLooping{ false };
	bool isPaused{ false };

	/// <summary>
	/// 1 while animation is on the last frame
	/// </summary>
	bool isAnimCycleComplete{ false };

	double animTimer{};
	//Used for selecting which frame to play.
	unsigned animCurrFrame{};
	//Calls once per frame. Args is the current frame that's playing, and if that frame is the endFrame
	//For animation events.
	void (*animFrameCallback)(unsigned frame, bool isDone) {nullptr};
};

