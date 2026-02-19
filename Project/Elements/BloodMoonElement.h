#ifndef _BLOOD_MOON_ELEMENT_H_
#define _BLOOD_MOON_ELEMENT_H_
#include "../GameObjects/GameObject.h"
class Actor;

namespace Elements {
	extern void BloodMoonEffect(GameObject::CollisionData& target, Actor* caster);
}

#endif // !_BLOOD_MOON_ELEMENT_H_
