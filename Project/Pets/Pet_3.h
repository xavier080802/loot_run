#ifndef _PET_3_
#define _PET_3_
#include "Pet.h"
class Actor;
class EquipmentData;

/*  Lycan

	extra:
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
class Pet_3 : public Pet
{
    GO_TYPE GetPetGOType() const override { return GO_TYPE::PET_3; }
    void Setup(Player& player)                         override;
    bool DoSkill(const Pets::SkillCastData& _data)    override;
    void SkillUpdate(float dt)                         override;
    void DoMovement(double dt)                         override;
    void Attack();

    Player* player{ nullptr };
    Actor* target{ nullptr };
    EquipmentData const* weap{ nullptr };

	Player* player{ nullptr };
	Actor* target{ nullptr };
	EquipmentData const* weap{ nullptr };
	float attackTimer{}, attackCooldown{};

	float attackAnimTimer{}, attImgTime{};
};
#endif // !_PET_3_