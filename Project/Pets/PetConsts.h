#ifndef _PET_CONSTS_H_
#define _PET_CONSTS_H_
#include <string>
#include <array>
#include <vector>
#include <map>
#include "../Actor/StatsTypes.h"
#include "../Actor/StatusEffect.h"
#include "../Elements/Element.h"
class Player;

namespace Pets {
	enum PET_RANK {
		COMMON,
		UNCOMMON,
		RARE,
		EPIC,
		LEGENDARY, 
		MYTHICAL   
	};

	// ID for all the pets
	enum PET_TYPE {
		NONE,
		PET_1, // Rock
		PET_2, // Slime
		PET_3, // Wolf
		PET_4, // Whale
		PET_5, // Garuda
		PET_6, // Dragon
		NUM_PETS // Last
	};

	struct PetData {
		~PetData() = default;
		PET_TYPE id{};
		std::string name{};
		StatEffects::StatusEffect passive{nullptr, -1, 1, "Pet"}; //Load from an array of Mods
		std::vector<StatEffects::Mod> multipliers{}; //Skill/Passive extra multipliers
		std::vector<DAMAGE_TYPE> dmgTypes{};
		std::array<float, 6> rarityScaling{}; //For each rarity, the mults/statEffect values * rarityScaling[rarity]
		float skillCooldown{};
		std::string skillDesc{};
		std::vector<std::string> textures{};
		std::vector<Elements::ELEMENT_TYPE> skillElements{};
		std::vector<StatEffects::StatusEffect> extraEffects{};
		std::map<std::string, std::string> extra{};
	};

	struct PetSaveData {
		PET_TYPE id{};
		PET_RANK rank{};

		PetSaveData(PET_TYPE _id = PET_TYPE::NONE, PET_RANK _rank = PET_RANK::COMMON)
			: id{ _id }, rank{ _rank } {
		};
	};

	struct SkillCastData {
		Player* player;
	};
}
#endif // !_PET_CONSTS_H_