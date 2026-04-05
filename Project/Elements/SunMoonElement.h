#ifndef SUNMOONELEMENT_H_
#define SUNMOONELEMENT_H_

#include "../GameObjects/GameObject.h"
#include "../GameObjects/AttackHitboxGO.h"
#include "Element.h"
class Actor;
struct EquipmentData;

namespace Elements {
	//Slows enemies, and reapplies all their debuffs periodically.
	//For owner, instead reapplies all buffs periodically.
	extern void SunMoonEffect(GameObject::CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE element, float knockback, EquipmentData* weapon = nullptr, void* ex = nullptr);

	//When object lifetime ends, detonate to deal damage to enemies.
	extern void SunMoonDetonate(AttackHitboxGO& hitbox, Actor* caster);
}

#endif // SUNMOONELEMENT_H_

