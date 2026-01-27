#ifndef _PETS_H_
#define _PETS_H_
#include "AEEngine.h"
#include <string>
#include <queue>
#include "../GameObjects/GameObject.h"
#include "PetConsts.h"
#include "PetSkills.h"
#include "../Actor/StatusEffect.h"

//This is the equipped pet during a run
class Pet : public GameObject
{
public:
	struct PetData {
		std::string name;
		float skillCooldown;
		bool (*PetSkill)(const PetSkills::SkillCastData& data);
		std::string texture;
		//TODO: buff values/effect
		StatEffects::StatusEffect passive{nullptr, -1, 1, ""};
	};

	void CastSkill(const PetSkills::SkillCastData& data);

	//Update pet logic (like skill cooldowns)
	void Update(double dt) override;

	void SetData(const PetData& newData);
	const PetData& GetPetData();

	//Reset game-time variables
	void Reset();

	bool isSet{ false };
private:
	PetData data{};
	int dupeLevel{};

	//Skill stuff
	float cooldownTimer{};

	//Movement
	AEVec2 targetPos{};
	std::queue<AEVec2> path;
};
#endif // !_PETS_H_
