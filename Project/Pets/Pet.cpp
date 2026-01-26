#include "Pet.h"
#include "PetManager.h"
#include "../Helpers/Vec2Utils.h"
#include <iostream>

namespace {
	//Distance from player to stop moving at.
	const float playerStopDist{50.f};
	//Speed of pet when moving
	const float petMoveSpeed{ 300.f };
	//Distance from a point to consider it as "at" that point
	const float pointTolerance{ 10 };
}

void Pet::CastSkill(const PetSkills::SkillCastData& skillData)
{
	if (cooldownTimer > 0 || !data.PetSkill) return;
	if (data.PetSkill(skillData)) {
		//Skill casted successfully.
		cooldownTimer = data.skillCooldown;
	}
}

void Pet::Update(double dt) {
	GameObject::Update(dt);
	if (cooldownTimer > 0) {
		cooldownTimer -= static_cast<float>(dt);
	}

	//Movement

	GameObject const& player = PetManager::GetInstance()->GetPlayer();

	AEVec2 playerPos = player.GetPos();
	bool isNearPlayer = AEVec2SquareDistance(&pos, &playerPos) <= playerStopDist * playerStopDist;
	//Not near player and last path point is sufficiently far
	if (!isNearPlayer && AEVec2SquareDistance(&playerPos, path.empty() ? &pos : &path.back()) >= pointTolerance * pointTolerance) {
		//Record player path
		path.push(playerPos);
	}

	//Follow path to player
	if (!isNearPlayer) {
		//Set targetPos to the next path point.
		targetPos = path.empty() ? playerPos : path.front();
		AEVec2 step{ targetPos.x - pos.x, targetPos.y - pos.y };
		AEVec2 stepNorm{};
		if (step.x || step.y) { //Prevent NaN issue if step is 0,0
			AEVec2Normalize(&stepNorm, &step);
		}
		pos.x += stepNorm.x * petMoveSpeed * static_cast<float>(dt);
		pos.y += stepNorm.y * petMoveSpeed * static_cast<float>(dt);

		//If near targetPos (reached path point), pop path point
		if (!path.empty() && AEVec2SquareDistance(&pos, &targetPos) <= pointTolerance) path.pop();
	}
	else if (!path.empty()){
		//Near player again. Clear path to prevent funny movement when player leaves vicinity.
		size_t temp{ path.size() }; //Dont put path.size() into loop.
		for (unsigned i = 0; i < temp; i++) {
			path.pop();
		}
	}
}

void Pet::SetData(const PetData& newData)
{
	data = newData;
}
