#ifndef PET_3_H_
#define PET_3_H_

#include "Pet.h"
#include "../Actor/ActorSubscriptions.h"
class Actor;
struct EquipmentData;

/*	Lycan

	When skill is cast, one enemy that is under the cursor
	will become the target.
	Pet goes over to the target to smack them.
	Damage is based on player's attack.

	extra:
		baseDmgMult - (float) Starting multiplier applied to attack damage. Scales with rarity scaling
		attackCooldown - (float) Time between attacks
		attImgTime - (float) Time to show the attack texture

	Pet's attack data is in melee.json, the unique weapon with id 7
	 - Attack size
	 - Element
	 - Knockback
	 - Etc

	Using a melee weapon to reuse Combat code

	textures
		0: Base texture
		1: Attack texture that appears when attacking
*/
class Pet_3 : public Pet, public ActorBeforeDealingDmgSub
{
	GO_TYPE GetPetGOType() const override { return GO_TYPE::PET_3; }
	void Setup(Player& player) override;
	bool DoSkill(const Pets::SkillCastData& _data) override;
	void SkillUpdate(float dt) override;
	void DoMovement(double dt) override;

	void Attack();

	void SubscriptionAlert(BeforeDealingDmgContent content) override;

	Player* player{ nullptr };
	Actor* target{ nullptr };
	EquipmentData const* weap{ nullptr };
	float attackTimer{}, attackCooldown{}, dmgMult{};

	float attackAnimTimer{}, attImgTime{};
};

#endif // PET_3_H_

