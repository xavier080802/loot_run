#ifndef _PETS_H_
#define _PETS_H_
#include <string>
#include "PetConsts.h"
#include "../GameObjects/GameObject.h"
#include "PetSkills.h"
#include "AEEngine.h"
#include <queue>

//This is the equipped pet during a run
class Pet : public GameObject
{
public:
	struct PetData {
		std::string name;
		float skillCooldown;
		bool (*PetSkill)(const PetSkills::SkillCastData& data);
		//TODO: buff values/effect
	};

	void CastSkill(const PetSkills::SkillCastData& data);

	//Update pet logic (like skill cooldowns)
	void Update(double dt) override;

	void SetData(const PetData& newData);

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
