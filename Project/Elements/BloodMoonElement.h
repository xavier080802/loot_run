#ifndef BLOODMOONELEMENT_H_
#define BLOODMOONELEMENT_H_

#include "../GameObjects/GameObject.h"
#include "Element.h"
class Actor;
struct EquipmentData;

namespace Elements {
	//Reaction between blood and moon.
	//Creates an object on the ground
	//Periodically damages and applies a def-down debuff.
	//This function does the periodic dmg and debuffing, called by the hitbox object
	extern void BloodMoonEffect(GameObject::CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE element, float knockback, EquipmentData* weapon=nullptr, void* extra = nullptr);
}

#endif // BLOODMOONELEMENT_H_

