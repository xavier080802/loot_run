#ifndef _ELEMENT_H_
#define _ELEMENT_H_
#include "../Actor/StatusEffect.h"
#include "AEEngine.h"
#include <string>
#include <map>
#include "../Helpers/ColorUtils.h"

//NOTE: Declare extern and define in Element.cpp, otherwise linker error :<

namespace Elements {
	enum class ELEMENT_TYPE {
		NONE,
		BLOOD,
		SUN,
		MOON,
	};

	//Load values from json
	extern bool InitElementalSystem();

	//Static function to apply element to a target.
	//Returns whether the element was applied or not
	extern bool ApplyElement(ELEMENT_TYPE ele, Actor* applier, Actor* target);

	//Checks if eff reacts with any other element in the dictionary.
	//If no, effectively does nothing
	//If reacts, function will handle reaction effect entirely.
	extern bool CheckReaction(Actor* actor, Actor* caster, std::map<std::string, StatEffects::StatusEffect*>& dict,
		StatEffects::StatusEffect* eff);

	//All element Settings
	extern float elementDur;

	//Blood
	extern std::string bloodName;
	extern std::vector<StatEffects::Mod> bloodDmgMods;
	extern std::string bloodIcon;

	//Sun
	extern std::string sunName;
	extern std::string sunBuffName;
	extern unsigned maxSunStacks;
	extern float sunBuffDur;
	extern unsigned sunLowRange, sunHighRange;
	extern std::vector<StatEffects::Mod> sunBuffMods;
	extern std::string sunIcon;

	//Moon
	extern std::string moonName;
	extern unsigned maxMoonStacks;
	extern std::vector<StatEffects::Mod> healMods;
	extern std::vector<StatEffects::Mod> moonKillHealMods;
	extern float moonMeleeHealMult;
	extern std::vector<StatEffects::Mod> moonDebuffMods;
	extern std::string moonIcon;

	//Blood+Sun reaction
	extern std::string bloodSunName;
	extern std::vector<StatEffects::Mod> bloodSunDotDmg;
	extern std::vector<StatEffects::Mod> bloodSunDetonateDmg;
	extern AEVec2 bloodSunDetoSize;
	extern std::string bloodSunIcon;

	//Blood+Moon reaction
	extern float bloodMoonLifetime;
	extern AEVec2 bloodMoonSize;
	extern float bloodMoonProcTime; //Time between procs
	extern float bloodMoonDebuffDur;
	extern unsigned bloodMoonDebuffMaxStacks;
	extern std::string bloodMoonDebuffName;
	extern std::vector<StatEffects::Mod> bloodMoonDebuffMods;
	extern std::vector<StatEffects::Mod> bloodMoonDmgMods;
	extern Color bloodMoonTint;

	//Sun+Moon reaction
	extern float sunMoonLifetime;
	extern AEVec2 sunMoonSize;
	extern float sunMoonProcTime;
	extern float sunMoonDebuffDur;
	extern unsigned sunMoonDebuffMaxStacks;
	extern std::string sunMoonDebuffName;
	extern std::vector<StatEffects::Mod> sunMoonDebuffMods;
	extern std::vector<StatEffects::Mod> sunMoonDmgMods;
	extern Color sunMoonTint;
}

#endif // !_ELEMENT_H_
