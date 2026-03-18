#ifndef _PET_3_
#define _PET_3_
#include "Pet.h"
class Actor;
class EquipmentData;

/*	Lycan
	
	When skill is cast, one enemy that is under the cursor
	will become the target.
	Pet goes over to the target to smack them.
	Damage is based on player's attack.

	extra:
		attackCooldown - (float) Time between attacks
*/
class Pet_3 : public Pet
{
	GO_TYPE GetPetGOType() const override { return GO_TYPE::PET_3; }
	void Setup(Player& player) override;
	bool DoSkill(const Pets::SkillCastData& _data) override;
	void SkillUpdate(float dt) override;
	void DoMovement(double dt) override;

	void Attack();

	Player* player{ nullptr };
	Actor* target{ nullptr };
	EquipmentData const* weap{ nullptr };
	float attackTimer{}, attackCooldown{};
};

#endif // !_PET_3_
