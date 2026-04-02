#include "StatsTypes.h"
#include <string>
#include <algorithm>

const char* StatTypeToString(STAT_TYPE stat) {
	switch (stat)
	{
	case MAX_HP:
		return "Max HP";
	case DEF:
		return "Def";
	case ATT:
		return "Att";
	case ATT_SPD:
		return "Att Spd";
	case MOVE_SPD:
		return "Spd";
	default:
		return "";
	}
}

STAT_TYPE ParseStatTypeFromStr(const char* str)
{
	//Change to uppercase first
	std::string cmp{ str };
	std::transform(cmp.begin(), cmp.end(), cmp.begin(), [](char c) {return static_cast<char>(std::toupper(c));});

	if (cmp == "HP" || cmp == "MAXHP" || cmp == "MAX HP" || cmp == "MAX_HP") return MAX_HP;
	if (cmp == "DEF") return DEF;
	if (cmp == "ATT" || cmp == "ATK") return ATT;
	if (cmp == "ATT_SPD" || cmp == "ATT SPD" || cmp == "ATTSPD") return ATT_SPD;
	if (cmp == "SPD" || cmp == "MOVESPD" || cmp == "MOVE SPD") return MOVE_SPD;

	return STAT_TYPE::ATT;
}

DAMAGE_TYPE ParseDmgTypeFromStr(const char* str)
{
	//Change to uppercase first
	std::string cmp{ str };
	std::transform(cmp.begin(), cmp.end(), cmp.begin(), [](char c) {return static_cast<char>(std::toupper(c));});

	if (cmp.find("PHY") != std::string::npos) return DAMAGE_TYPE::PHYSICAL;
	if (cmp.find("MAG") != std::string::npos) return DAMAGE_TYPE::MAGICAL;
	if (cmp.find("ELE") != std::string::npos) return DAMAGE_TYPE::ELEMENTAL;
	if (cmp.find("TRUE") != std::string::npos) return DAMAGE_TYPE::TRUE_DAMAGE;

	return DAMAGE_TYPE::PHYSICAL; 
}
