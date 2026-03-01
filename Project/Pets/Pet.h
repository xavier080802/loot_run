#ifndef _PETS_H_
#define _PETS_H_
#include "AEEngine.h"
#include <string>
#include <queue>
#include "../GameObjects/GameObject.h"
#include "PetConsts.h"
#include "PetSkills.h"
#include "../Actor/StatusEffect.h"
#include <initializer_list>

//This is the equipped pet during a run
class Pet : public GameObject
{
public:
	void CastSkill(const PetSkills::SkillCastData& data);

	//Update pet logic (like skill cooldowns)
	void Update(double dt) override;

	//[0] being the start of the new path.
	//Set append to true to add to existing path instead of clearing.
	void SetPath(std::initializer_list<AEVec2> const& _path, bool append = false);

	void SetData(const Pets::PetData& newData, Pets::PET_RANK rank);
	const Pets::PetData& GetPetData();
	//Get specific multiplier from pet data
	StatEffects::Mod const& GetMultiplier(unsigned index = 0);
	//Get specific skill element from pet data
	Elements::ELEMENT_TYPE GetSkillElement(unsigned index = 0);
	bool IsOnCooldown() const { return cooldownTimer > 0.f; }
	float GetCDTimer() const { return cooldownTimer; }

	void ClearPath();
	//Reset game-time variables
	void Reset();

	virtual ~Pet() {};

	bool isSet{ false };
protected:
	void MoveToTarget(double dt);

	Pets::PetData data{};

	//Skill stuff
	float cooldownTimer{};

	//Movement
	AEVec2 targetPos{};
	std::queue<AEVec2> path{};
	bool followPlayer{ true };
};
#endif // !_PETS_H_
