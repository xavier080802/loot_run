#ifndef _PET_CONSTS_H_
#define _PET_CONSTS_H_
#include <string>
#include <array>
#include <vector>
#include "../Actor/StatsTypes.h"
#include "../Actor/StatusEffect.h"
#include "../Elements/Element.h"
#include "PetSkills.h"

namespace Pets {
	enum PET_RANK {
		COMMON,
		UNCOMMON,
		RARE,
		EPIC,
		MYTHICAL,
		LEGENDARY
	};

	//ID for all the pets
	enum PET_TYPE {
		NONE,
		PET_1,

		NUM_PETS //Last
	};

	//Data of a pet species, parsed from a json
	struct PetData {
		PET_TYPE id{};
		std::string name{};
		StatEffects::StatusEffect passive{nullptr, -1, 1,"Pet"}; //Load from an array of Mods
		std::vector<StatEffects::Mod> multipliers{}; //Skill/Passive extra multipliers
		std::array<float, 6> rarityScaling{}; //For each rarity, the mults/statEffect values * rarityScaling[rarity]
		float skillCooldown;
		std::string skillDesc;
		std::string texture;
		std::vector<Elements::ELEMENT_TYPE> skillElements; //Elements the skill may inflict (implementation-based)

		//Not loaded from json
		bool (*PetSkill)(const PetSkills::SkillCastData& data);
	};

	//Owned pets - CSV parse format
	struct PetSaveData {
		PET_TYPE id{};
		PET_RANK rank{};

		PetSaveData(PET_TYPE _id = PET_TYPE::NONE, PET_RANK _rank = PET_RANK::COMMON)
			: id{ _id }, rank{ _rank } {};
	};
}
#endif // !_PET_CONSTS_H_

