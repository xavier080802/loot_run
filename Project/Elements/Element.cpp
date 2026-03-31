#include "Element.h"
#include "../GameObjects/GameObjectManager.h"
#include "../Actor/Actor.h"
#include "../Actor/StatusEffect.h"
#include "../GameObjects/AttackHitboxGO.h"
#include "SunElement.h"
#include "BloodElement.h"
#include "MoonElement.h"
#include "BloodSunElement.h"
#include "BloodMoonElement.h"
#include "SunMoonElement.h"
#include "../Helpers/Vec2Utils.h"
#include "../Helpers/ColorUtils.h"

#include <iostream>
#include <fstream>
#include <json/json.h>

namespace Elements {

	//====================Extern variables====================
	
	//All element Settings
	float elementDur;

	//Blood
	std::string bloodName;
	std::vector<StatEffects::Mod> bloodDmgMods;
	std::string bloodIcon;
	Color bloodTextCol;

	//Sun
	std::string sunName;
	std::string sunBuffName;
	unsigned maxSunStacks;
	float sunBuffDur;
	unsigned sunLowRange, sunHighRange; //Range for number of stacks to apply
	std::vector<StatEffects::Mod> sunBuffMods;
	std::string sunIcon;
	Color sunTextCol;

	//Moon
	std::string moonName;
	unsigned maxMoonStacks;
	std::vector<StatEffects::Mod> healMods;
	std::vector<StatEffects::Mod> moonKillHealMods;
	float moonMeleeHealMult{};
	std::vector<StatEffects::Mod> moonDebuffMods;
	std::string moonIcon;
	Color moonTextCol;

	//Blood+Sun reaction
	std::string bloodSunName;
	std::vector<StatEffects::Mod> bloodSunDotDmg;
	std::vector<StatEffects::Mod> bloodSunDetonateDmg;
	AEVec2 bloodSunDetoSize;
	std::string bloodSunIcon;
	Color bloodSunDetoColor;

	//Blood+Moon reaction
	float bloodMoonLifetime;
	AEVec2 bloodMoonSize;
	float bloodMoonProcTime; //Time between procs
	std::string bloodMoonDebuffName;
	float bloodMoonDebuffDur;
	unsigned bloodMoonDebuffMaxStacks;
	std::vector<StatEffects::Mod> bloodMoonDebuffMods;
	std::vector<StatEffects::Mod> bloodMoonDmgMods;
	Color bloodMoonTint;

	//Sun+Moon reaction
	float sunMoonLifetime;
	AEVec2 sunMoonSize;
	float sunMoonProcTime;
	std::string sunMoonDebuffName;
	float sunMoonDebuffDur;
	unsigned sunMoonDebuffMaxStacks;
	std::vector<StatEffects::Mod> sunMoonDebuffMods;
	std::vector<StatEffects::Mod> sunMoonDmgMods;
	Color sunMoonTint;

	//==========================================================================================================//

	//Load settings from json
	bool InitElementalSystem()
	{
		std::ifstream ifs{ "Assets/Data/elements.json", std::ios_base::binary };
		if (!ifs.is_open()) {
			std::cout << "FAILED TO OPEN ELEMENTS JSON\n";
			return false;
		}
		Json::Value root;
		Json::CharReaderBuilder builder;
		std::string errs;

		if (!Json::parseFromStream(builder, ifs, &root, &errs))
		{
			std::cout << "Elemental system data: parse failed: " << errs << "\n";
			return false;
		}
		if (!root.isObject()) {
			std::cout << "Elemental system data: missing/invalid root\n";
			return false;
		}

		//Parse global values
		Json::Value global{ root["global"] };
		elementDur = global.get("elementDur", 1).asFloat();

		//Parse blood settings
		Json::Value blood{ root["blood"] };
		bloodName = blood.get("name", "Blood DoT").asString();
		for (Json::Value const& m : blood["dmgMods"]) {
			bloodDmgMods.push_back(StatEffects::Mod::ParseFromJSON(m));
		}
		bloodIcon = blood.get("icon", "").asString();
		if (blood["textCol"].isArray() && blood["textCol"].size() == 4) {
			bloodTextCol = Color{
				blood["textCol"][0].asFloat(), blood["textCol"][1].asFloat(),
				blood["textCol"][2].asFloat(),blood["textCol"][3].asFloat()
			};
		}

		//Parse Sun
		Json::Value sun{ root["sun"] };
		sunName = sun.get("name", "Sun").asString();
		sunBuffName = sun.get("buffName", "Sun's Buff").asString();
		maxSunStacks = sun.get("maxStacks", 1).asUInt();
		sunBuffDur = sun.get("buffDur", 0).asFloat();
		sunLowRange = sun.get("lowRange", 0).asUInt();
		sunHighRange = sun.get("highRange", 1).asUInt();
		for (Json::Value const& m : sun["buffMods"]) {
			sunBuffMods.push_back(StatEffects::Mod::ParseFromJSON(m));
		}
		sunIcon = sun.get("icon", "").asString();
		if (sun["textCol"].isArray() && sun["textCol"].size() == 4) {
			sunTextCol = Color{
				sun["textCol"][0].asFloat(), sun["textCol"][1].asFloat(),
				sun["textCol"][2].asFloat(),sun["textCol"][3].asFloat()
			};
		}

		//Parse Moon
		Json::Value moon{ root["moon"] };
		moonName = moon.get("name", "Moon").asString();
		maxMoonStacks = moon.get("maxStacks", 1).asUInt();
		for (Json::Value const& m : moon["healMods"]) {
			healMods.push_back(StatEffects::Mod::ParseFromJSON(m));
		}
		moonMeleeHealMult = moon.get("meleeHealMult", 1).asFloat();
		for (Json::Value const& m : moon["killHealMods"]) {
			moonKillHealMods.push_back(StatEffects::Mod::ParseFromJSON(m));
		}
		for (Json::Value const& m : moon["debuffMods"]) {
			moonDebuffMods.push_back(StatEffects::Mod::ParseFromJSON(m));
		}
		moonIcon = moon.get("icon", "").asString();
		if (moon["textCol"].isArray() && moon["textCol"].size() == 4) {
			moonTextCol = Color{
				moon["textCol"][0].asFloat(), moon["textCol"][1].asFloat(),
				moon["textCol"][2].asFloat(),moon["textCol"][3].asFloat()
			};
		}

		//Parse Blood+Sun reaction
		Json::Value bloodSun{ root["blood_sun"] };
		bloodSunName = bloodSun.get("name", "Blood-Sun").asString();
		for (Json::Value const& m : bloodSun["dotDmgMods"]) {
			bloodSunDotDmg.push_back(StatEffects::Mod::ParseFromJSON(m));
		}
		for (Json::Value const& m : bloodSun["detonateDmgMods"]) {
			bloodSunDetonateDmg.push_back(StatEffects::Mod::ParseFromJSON(m));
		}
		bloodSunDetoSize.x = bloodSun.get("detonateSize_x", 0).asFloat();
		bloodSunDetoSize.y = bloodSun.get("detonateSize_y", 0).asFloat();
		bloodSunIcon = bloodSun.get("icon", "").asString();
		Json::Value _bloodSunTint = bloodSun["detonationCol"];
		if (_bloodSunTint.isArray() && _bloodSunTint.size() == 4) {
			bloodSunDetoColor = Color{
				_bloodSunTint[0].asFloat(), _bloodSunTint[1].asFloat(),
				_bloodSunTint[2].asFloat(),_bloodSunTint[3].asFloat()
			};
		}

		//Parse Blood+Moon
		Json::Value bloodMoon{ root["blood_moon"] };
		bloodMoonLifetime = bloodMoon.get("lifetime", 0).asFloat();
		bloodMoonSize.x = bloodMoon.get("size_x", 0).asFloat();
		bloodMoonSize.y = bloodMoon.get("size_y", 0).asFloat();
		bloodMoonProcTime = bloodMoon.get("procTime", 0).asFloat();
		bloodMoonDebuffDur = bloodMoon.get("debuffDur", 0).asFloat();
		bloodMoonDebuffName = bloodMoon.get("debuffName", "Blood Moon Debuff").asString();
		for (Json::Value const& m : bloodMoon["debuffMods"]) {
			bloodMoonDebuffMods.push_back(StatEffects::Mod::ParseFromJSON(m));
		}
		for (Json::Value const& m : bloodMoon["dmgMods"]) {
			bloodMoonDmgMods.push_back(StatEffects::Mod::ParseFromJSON(m));
		}
		Json::Value _bloodMoonTint = bloodMoon["tint"];
		bloodMoonTint = Color{ _bloodMoonTint[0].asFloat(), _bloodMoonTint[1].asFloat() , _bloodMoonTint[2].asFloat() , _bloodMoonTint[3].asFloat() };

		//Parse Sun+Moon reaction
		Json::Value sunMoon{ root["sun_moon"] };
		sunMoonLifetime = sunMoon.get("lifetime", 0).asFloat();
		sunMoonSize.x = sunMoon.get("size_x", 0).asFloat();
		sunMoonSize.y = sunMoon.get("size_y", 0).asFloat();
		sunMoonProcTime = sunMoon.get("procTime", 1.f).asFloat();
		sunMoonDebuffName = sunMoon.get("debuffName", "Sun Moon Debuff").asString();
		sunMoonDebuffDur = sunMoon.get("debuffDur", 0).asFloat();
		sunMoonDebuffMaxStacks = sunMoon.get("debuffMaxStacks", 1).asUInt();
		for (Json::Value const& m : sunMoon["debuffMods"]) {
			sunMoonDebuffMods.push_back(StatEffects::Mod::ParseFromJSON(m));
		}
		for (Json::Value const& m : sunMoon["dmgMods"]) {
			sunMoonDmgMods.push_back(StatEffects::Mod::ParseFromJSON(m));
		}
		Json::Value _sunMoonTint = sunMoon["tint"];
		sunMoonTint = Color{ _sunMoonTint[0].asFloat(), _sunMoonTint[1].asFloat() , _sunMoonTint[2].asFloat() , _sunMoonTint[3].asFloat() };

		ifs.close();
		return true;
	}

	//Static function to apply a basic element
	bool ApplyElement(ELEMENT_TYPE ele, Actor* applier, Actor* target)
	{
		if (ele == ELEMENT_TYPE::NONE) return false;

		StatEffects::StatusEffect* se{ nullptr };
		switch (ele)
		{
		case ELEMENT_TYPE::BLOOD:
			se = new BloodElement{ Elements::bloodName }; //Note: when applied via Eclipse, use other name
			std::cout << "APPLIED BLOOD\n";
			break;
		case ELEMENT_TYPE::SUN: {
			//Apply random number of Sun stacks
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
				|| second == first) continue;
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
				AttackHitboxGO* hb{ dynamic_cast<AttackHitboxGO*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::ATTACK_HITBOX)) };
				if (hb) {
					AttackHitboxConfig cfg{ };
					cfg.owner = caster;
					cfg.colliderSize = cfg.renderScale = bloodMoonSize;
					cfg.tint = bloodMoonTint;
					cfg.lifetime = bloodMoonLifetime;
					cfg.disableOnHit = cfg.followOwner = false;
					cfg.hitCooldown = bloodMoonProcTime;
					cfg.onHit = BloodMoonEffect;
					cfg.zIndex = -2;
					hb->Start(cfg);
				}
				std::cout << "REACTION BLOOD MOON\n";
			}
			else if ((first == StatEffects::EFF_TYPE::MOON) && (second == StatEffects::EFF_TYPE::SUN)
				|| (first == StatEffects::EFF_TYPE::SUN) && (second == StatEffects::EFF_TYPE::MOON)) {
				//Sun + Moon: Place Eclipse object.
				AttackHitboxGO* hb{ dynamic_cast<AttackHitboxGO*>(GameObjectManager::GetInstance()->FetchGO(GO_TYPE::ATTACK_HITBOX)) };
				if (hb) {
					AttackHitboxConfig cfg{ };
					cfg.owner = caster;
					cfg.offset = actor->GetPos() - caster->GetPos(); //Place on victim, not caster
					cfg.colliderSize = cfg.renderScale = sunMoonSize;
					cfg.tint = sunMoonTint; 
					cfg.lifetime = sunMoonLifetime;
					cfg.disableOnHit = cfg.followOwner = false;
					cfg.hitCooldown = sunMoonProcTime;
					cfg.onHit = SunMoonEffect;
					cfg.onEnd = SunMoonDetonate;
					cfg.zIndex = -2;
					hb->Start(cfg);
					//Change collision layer to include caster
					Bitmask bm = caster->GetCollisionLayers();
					SetFlagAtPos(&bm, caster->GetColliderLayer());
					hb->SetCollisionLayers(bm);
				}
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
	std::string GetElementName(ELEMENT_TYPE ele)
	{
		switch (ele)
		{
		case Elements::ELEMENT_TYPE::BLOOD:
			return "Blood";
		case Elements::ELEMENT_TYPE::SUN:
			return "Sun";
		case Elements::ELEMENT_TYPE::MOON:
			return "Moon";
		default:
			return "None";
		}
	}
	Color const& GetElementCol(ELEMENT_TYPE ele)
	{
		switch (ele)
		{
		case Elements::ELEMENT_TYPE::BLOOD:
			return bloodTextCol;
		case Elements::ELEMENT_TYPE::SUN:
			return sunTextCol;
		case Elements::ELEMENT_TYPE::MOON:
			return moonTextCol;
		default:
			return {255,255,255,255};
		}
	}
}


