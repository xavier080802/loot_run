#ifndef _BLOOD_MOON_ELEMENT_H_
#define _BLOOD_MOON_ELEMENT_H_
#include "../GameObjects/GameObject.h"
#include "Element.h"
class Actor;
struct EquipmentData;

namespace Elements {
	//Reaction between blood and moon.
	//Creates an object on the ground
	//Periodically damages and applies a def-down debuff.
	extern void BloodMoonEffect(GameObject::CollisionData& target, Actor* caster, Elements::ELEMENT_TYPE element, float knockback);
}

#endif // !_BLOOD_MOON_ELEMENT_H_
