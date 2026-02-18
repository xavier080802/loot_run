#ifndef _ELEMENT_H_
#define _ELEMENT_H_
#include "../Actor/StatusEffect.h"
#include <string>
#include <map>

//NOTE: Declare extern and define in Element.cpp, otherwise linker error :<

namespace Elements {
	enum class ELEMENT_TYPE {
		NONE,
		BLOOD,
		SUN,
		MOON,
	};
	//Static function to apply element to a target.
	//Returns whether the element was applied or not
	extern bool ApplyElement(ELEMENT_TYPE ele, Actor* applier, Actor* target);

	//Checks if eff reacts with any other element in the dictionary.
	//If no, effectively does nothing
	//If reacts, function will handle reaction effect entirely.
	extern bool CheckReaction(Actor* actor, std::map<std::string, StatEffects::StatusEffect*>& dict, StatEffects::StatusEffect* eff);

	//All element Settings
	extern float elementDur;

	//Blood
	extern const std::string bloodName;

	//Sun
	extern const std::string sunName;
	extern const std::string sunBuffName;
	extern unsigned maxSunStacks;
	extern float sunBuffDur;
	extern unsigned sunLowRange, sunHighRange;

	//Moon
	extern const std::string moonName;
	extern unsigned maxMoonStacks;
}

#endif // !_ELEMENT_H_
