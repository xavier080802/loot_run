#ifndef _BLOOD_MOON_ELEMENT_H_
#define _BLOOD_MOON_ELEMENT_H_
#include "../GameObjects/GameObject.h"
class Actor;

namespace Elements {
	//Reaction between blood and moon.
	//Creates an object on the ground
	//Periodically damages and applies a def-down debuff.
	extern void BloodMoonEffect(GameObject::CollisionData& target, Actor* caster, void* = nullptr);
}

#endif // !_BLOOD_MOON_ELEMENT_H_
