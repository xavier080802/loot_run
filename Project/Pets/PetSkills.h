#ifndef _PET_SKILLS_H_
#define _PET_SKILLS_H_

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
		Player const* player;
		Pet const* thisPet;
	};

	bool Pet1Skill(const SkillCastData& data);
}
#endif // !_PET_SKILLS_H_


