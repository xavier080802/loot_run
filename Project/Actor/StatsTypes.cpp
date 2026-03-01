#include "StatsTypes.h"

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