#ifndef _SUN_MOON_ELEMENT_H_
#define _SUN_MOON_ELEMENT_H_
#include "../GameObjects/GameObject.h"
#include "Element.h"
class Actor;
struct EquipmentData;

namespace Elements {
	//Slows enemies, and reapplies all their debuffs periodically.
	//For owner, instead reapplies all buffs periodically.
	extern void SunMoonEffect(GameObject::CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE element, float knockback, EquipmentData* weapon = nullptr, void* ex = nullptr);

	//When object lifetime ends, detonate to deal damage to enemies.
	extern void SunMoonDetonate(Actor* caster);
}

#endif // !_SUN_MOON_ELEMENT_H_
