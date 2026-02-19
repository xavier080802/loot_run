#include "Element.h"
#include "../Actor/Actor.h"
#include "../Actor/StatusEffect.h"
#include "SunElement.h"
#include "BloodElement.h"
#include "MoonElement.h"
#include "BloodSunElement.h"

#include <iostream>

namespace Elements {
	//All element Settings
	float elementDur{ 15.f };

	//Blood
	const std::string bloodName{ "Bloodletting" };

	//Sun
	const std::string sunName{ "Sun" };
	const std::string sunBuffName{ "Sun's Buff" };
	unsigned maxSunStacks{ 20 };
	float sunBuffDur{ 7.5f };
	unsigned sunLowRange{ 3 }, sunHighRange{ 5 }; //Range for number of stacks to apply

	//Moon
	const std::string moonName{ "Wax and Wane" };
	unsigned maxMoonStacks{ 5 };

	//Blood+Sun reaction
	std::string bloodSunName{"Boiling Blood"};

	//Blood+Moon reaction
	

	//Sun+Moon reaction


	//==========================================================================================================//

	//Static function to apply an element
	bool ApplyElement(ELEMENT_TYPE ele, Actor* applier, Actor* target)
	{
		StatEffects::StatusEffect* se{ nullptr };
		switch (ele)
		{
		case ELEMENT_TYPE::BLOOD:
			se = new BloodElement{ Elements::bloodName }; //Note: when applied via Eclipse, use other name
			std::cout << "APPLIED BLOOD\n";
			break;
		case ELEMENT_TYPE::SUN: {
			unsigned s{ (rand() % (Elements::sunHighRange - Elements::sunLowRange + 1)) + Elements::sunLowRange };
			se = new SunElement(s);
			std::cout << "APPLIED SUN " << s << '\n';
			break;
		}
		case ELEMENT_TYPE::MOON:
			se = new MoonElement{};
			std::cout << "APPLIED MOON\n";
			break;
		default:
			return false;
		}
		target->ApplyStatusEffect(se, applier);
		return true;
	}

	bool CheckReaction(Actor* actor, Actor* caster, std::map<std::string, StatEffects::StatusEffect*>& dict, StatEffects::StatusEffect* eff)
	{
		StatEffects::EFF_TYPE first{ eff->GetType() };
		if (eff->GetType() <= StatEffects::EFF_TYPE::DEBUFF) return false;
		//Find other element (if any)
		for (auto& pair : dict) {
			StatEffects::EFF_TYPE second{ pair.second->GetType() };
			//Skip non-reaction element and same type
			if (second <= StatEffects::EFF_TYPE::DEBUFF
				|| second == eff->GetType()) continue;
			//Found. Find reaction type.
			if ((first == StatEffects::EFF_TYPE::BLOOD) && (second == StatEffects::EFF_TYPE::SUN)
				|| (first == StatEffects::EFF_TYPE::SUN) && (second == StatEffects::EFF_TYPE::BLOOD)) {
				//Blood + Sun: Add Boiling Blood effect.
				actor->ApplyStatusEffect(new BloodSunElement(Elements::bloodSunName), caster);
				std::cout << "REACTION BLOOD SUN\n";
			}
			else if ((first == StatEffects::EFF_TYPE::BLOOD) && (second == StatEffects::EFF_TYPE::MOON)
				|| (first == StatEffects::EFF_TYPE::MOON) && (second == StatEffects::EFF_TYPE::BLOOD)) {
				//Blood + Moon: Place moon object.
				std::cout << "REACTION BLOOD MOON\n";
			}
			else if ((first == StatEffects::EFF_TYPE::MOON) && (second == StatEffects::EFF_TYPE::SUN)
				|| (first == StatEffects::EFF_TYPE::SUN) && (second == StatEffects::EFF_TYPE::MOON)) {
				//Sun + Moon: Place Eclipse object.
				std::cout << "REACTION SUN MOON\n";

			}
			else return false; //No match.

			//Did match, reaction true. Delete base element status effects involved.
			delete pair.second;
			dict.erase(pair.first);
			delete eff;
			return true;
		}
		//Failed to find other element
		return false;
	}

}


