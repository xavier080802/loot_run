#ifndef _PET_6_H_
#define _PET_6_H_
#include "Pet.h"

/*	Dragon
	
	Passively fires at nearby enemies (referred to in code as Turret).
		- Proj is piercing (doesnt die on first collision)
	Skill: Knock back and damage surrounding enemies

	multipliers:
		[0] and [1] - Turret damage
		[2] and [3] - Skill damage
		
	extra:
		knockback - (float) Strength of the skill's knockback
		skillRange - (float) Range of skill (the knockback)
		attackCooldown - (float) Time between turret shots
		turretRange - (float) Turret range (radius)
		projSpd - (float) Speed of the turret projectile
		projSize - (float) Diameter of the turret projectile
		projLife - (float) Lifetime of turret projectile
		projCol - "r g b a" Color of turret proj
*/
class Pet_6 : public Pet
{
public:
	void Setup(Player& player) override;

private:
	GO_TYPE GetPetGOType() const override { return GO_TYPE::PET_6; }
	bool DoSkill(const Pets::SkillCastData& _data) override;
	void SkillUpdate(float dt) override;

	float attackTimer{}, attackCooldown{}, knockback{}, skillRange{},
		turretRange{},
		projSpd{}, projSize{}, projLife{};
	Color projCol{};
	Player* player{};
};
#endif // !_PET_6_H_