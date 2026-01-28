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
	struct PetData {
		std::string name;
		float skillCooldown;
		bool (*PetSkill)(const PetSkills::SkillCastData& data);
		std::string texture;
		StatEffects::StatusEffect passive{nullptr, -1, 1, ""};
	};

	void CastSkill(const PetSkills::SkillCastData& data);

	//Update pet logic (like skill cooldowns)
	void Update(double dt) override;

	//[0] being the start of the new path.
	//Set append to true to add to existing path instead of clearing.
	void SetPath(std::initializer_list<AEVec2> const& _path, bool append = false);

	void SetData(const PetData& newData);
	const PetData& GetPetData();

	void ClearPath();
	//Reset game-time variables
	void Reset();

	bool isSet{ false };
protected:
	void MoveToTarget(double dt);

	PetData data{};
	int dupeLevel{};

	//Skill stuff
	float cooldownTimer{};

	//Movement
	AEVec2 targetPos{};
	std::queue<AEVec2> path;
	bool followPlayer{ true };
};
#endif // !_PETS_H_
