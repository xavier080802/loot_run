#ifndef _PET_SKILLS_H_
#define _PET_SKILLS_H_
#include <vector>
//Skill function signature:
// Returns whether the skill cast successfully or not
// bool PetSkill(const SkillCastData& data);

//Avoiding circular dependency. Including in here will throw incoherent errors.
//#include in .cpp instead
class Player;
class Pet;

namespace PetSkills {
	//Stuff to pass to the cast function
	struct SkillCastData {
		Player* player;
		Pet const* thisPet;
	};

#pragma region Skill_Funcs
	//Pet failed to load
	bool PetNullSkill(const SkillCastData& data);

	bool Pet1Skill(const SkillCastData& data);

#pragma endregion

	//List of Pet skills. Ensure that skill order is same as PET_TYPE
	const std::vector<bool(*)(const SkillCastData&)> skills{
		PetNullSkill, // NONE
		Pet1Skill,
	};
}
#endif // !_PET_SKILLS_H_


