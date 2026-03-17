#include "Pet.h"
#include "PetManager.h"
#include "../Helpers/Vec2Utils.h"
#include "../TileMap.h"
#include <iostream>

namespace {
	//Distance from player to stop moving at.
	const float playerStopDist{50.f};
	//Speed of pet when moving
	const float petMoveSpeed{ 300.f };
	//Distance from a point to consider it as "at" that point
	const float pointTolerance{ 10 };
	//Max number of path nodes till pet is considered too far
	const unsigned maxPathLength{ 10 };
}

void Pet::CastSkill(const Pets::SkillCastData& skillData)
{
	if (cooldownTimer > 0 || !HasSkill()) return;
	if (DoSkill(skillData)) {
		//Skill casted successfully.
		cooldownTimer = data.skillCooldown;
	}
}

GameObject* Pet::Init(AEVec2 _pos, AEVec2 _scale, int _z, MESH_SHAPE _meshShape, Collision::SHAPE _colShape, AEVec2 _colSize, Bitmask _collideWithLayers, Collision::LAYER _isInLayer)
{
	goType = GetPetGOType();
	return GameObject::Init(_pos, _scale, _z, _meshShape, _colShape, _colSize, _collideWithLayers, _isInLayer);
}

void Pet::SetTilemap(TileMap const& map)
{
	tilemap = &map;
}

void Pet::Update(double dt) {
	GameObject::Update(dt);
	if (cooldownTimer > 0) {
		cooldownTimer -= static_cast<float>(dt);
	}

	//Handle pet pathfinding movement
	DoMovement(dt);

	SkillUpdate((float)dt);
}

void Pet::SetPath(std::initializer_list<AEVec2> const& _path, bool append)
{
	if (!append) {
		ResetPathfinder();
	}
	for (AEVec2 const& p : _path) {
		PushPath(p);
	}
}

void Pet::SetData(const Pets::PetData& newData, Pets::PET_RANK _rank)
{
	data = newData;
	rank = _rank;
	data.passive.SetName(data.name + "'s Call of Awakening");
	//Scale values based on rarity
	data.passive.ScaleMods(data.rarityScaling[_rank]);
	for (StatEffects::Mod& m : data.multipliers) {
		m.value *= data.rarityScaling[_rank];
	}
}

const Pets::PetData& Pet::GetPetData()
{
	return data;
}

StatEffects::Mod const& Pet::GetMultiplier(unsigned index)
{
	return data.multipliers.at(index);
}

Elements::ELEMENT_TYPE Pet::GetSkillElement(unsigned index)
{
	if (data.skillElements.empty()) return Elements::ELEMENT_TYPE::NONE;
	return data.skillElements.at(index);
}

void Pet::ClearPath()
{
	ResetPathfinder();
}

void Pet::Reset()
{
	ClearPath();
	targetPos = {};
	cooldownTimer = 0.f;
	followPlayer = true;
}

void Pet::DoMovement(double dt)
{
	UpdatePathfinding((float)dt); //Timers

	//If not following player, complete path.
	if (!followPlayer) {
		std::deque<AEVec2> const& path{ GetFoundPath() }; //Previous path
		targetPos = path.empty() ? pos : path.front();
		MoveToTarget(dt);
		return;
	}

	GameObject const& player = PetManager::GetInstance()->GetPlayer();
	AEVec2 playerPos = player.GetPos();
	float sqrDistFromPlayer{ AEVec2SquareDistance(&pos, &playerPos) };
	//Start pathfinding
	bool isNearPlayer = sqrDistFromPlayer <= playerStopDist * playerStopDist;
	if (!isNearPlayer && tilemap) {
		DoPathFinding(*tilemap, pos, playerPos);
		std::deque<AEVec2> const& path{ GetFoundPath() };
		//Pet too far from player (in terms of path nodes) - TP to player
		if (path.size() >= maxPathLength) {
			SetPos(playerPos);
			ResetPathfinder();
		}
		else {
			//Follow the path created by the pathfinder
			targetPos = path.empty() ? pos : path.front();
			MoveToTarget(dt);
		}
	}
	else if (isNearPlayer) { //No need to pathfind
		ResetPathfinder(); //Clear path
	}
	//DrawPath();
}

void Pet::MoveToTarget(double dt)
{
	AEVec2 step{ targetPos.x - pos.x, targetPos.y - pos.y };
	AEVec2 stepNorm{};
	if (step.x || step.y) { //Prevent NaN issue if step is 0,0
		AEVec2Normalize(&stepNorm, &step);
	}
	pos.x += stepNorm.x * petMoveSpeed * static_cast<float>(dt);
	pos.y += stepNorm.y * petMoveSpeed * static_cast<float>(dt);

	//If near targetPos (reached path point), pop path point
	//if (!path.empty() && AEVec2SquareDistance(&pos, &targetPos) <= pointTolerance) path.pop();
	if (AEVec2SquareDistance(&pos, &targetPos) <= pointTolerance) PopPath();
}

void Pet::Setup(Player&)
{
}

bool Pet::DoSkill(const Pets::SkillCastData&)
{
	return false;
}

void Pet::SkillUpdate(float)
{
}
