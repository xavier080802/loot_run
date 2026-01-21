#ifndef _PET_SKILLS_H_
#define _PET_SKILLS_H_

//Signature:
// Returns whether the skill cast successfully or not
// bool PetSkill(const SkillCastData& data);

namespace PetSkills {
	//Stuff to pass to the cast function
	struct SkillCastData {
	};

	bool Pet1Skills(const SkillCastData& data);
}
#endif // !_PET_SKILLS_H_


