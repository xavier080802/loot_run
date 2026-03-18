#ifndef _PET_2_H_
#define _PET_2_H_
#include "Pet.h"
#include "../Actor/ActorSubscriptions.h"

// Slime pet
// Skill:	Apply a shield to the player
//			If the player is hit while shielded, the attacker is debuffed.
//			If the player's curr shield val when hit is high enough, damage the attacker.
//
// Scalings:	Shield: multipliers[0] and [1]
//				Debuff: extraEffects [0]
//				Damage threshold: multipliers[2] (should be multiplicative)
//				Revenge dmg: multipliers[3] and [4]
class Pet_2 : public Pet, ActorOnHitSub
{
	GO_TYPE GetPetGOType() const override { return GO_TYPE::PET_2; }

	void Setup(Player& player) override;
	bool DoSkill(const Pets::SkillCastData& data) override;

	void SubscriptionAlert(OnHitContent content) override;

	float threshold{};
};

#endif // !_PET_2_H_
