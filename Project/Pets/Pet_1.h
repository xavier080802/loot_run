#ifndef _PET_1_H
#define _PET_1_H
#include "Pet.h"

// Rock pet
// Skill: Projectile that does damage.
// Scalings: Multiplier array of 2
class Pet_1 : public Pet
{
	GO_TYPE GetPetGOType() const override { return GO_TYPE::PET_1; }
	bool DoSkill(const Pets::SkillCastData& data) override;
};

#endif // !_PET_1_H
