#ifndef _STATS_TYPES_H_
#define _STATS_TYPES_H_

//TODO: make map instead. key - STAT_TYPE
struct ActorStats
{
	float maxHP = 0.0f;
	float attack = 0.0f;
	float defense = 0.0f;
	float moveSpeed = 0.0f; // pixels/sec (world coords)
	float attackSpeed = 0.0f; // attacks/sec
};

struct EquipmentModifiers
{
	ActorStats additive; // additive-only equipment stats
};

struct UpgradeMultipliers
{
	float hpMult = 1.0f;          // Multiplier for max HP. 1.0 = no change, 2.0 = double HP.
	float attackMult = 1.0f;      // Multiplier for attack damage.
	float defenseMult = 1.0f;     // Multiplier for defense.
	float moveSpeedMult = 1.0f;   // Multiplier for movement speed from the shop.
	float attackSpeedMult = 1.0f; // Multiplier for how fast the character attacks.
};

enum STAT_TYPE {
	MAX_HP,
	DEF,
	ATT,
	ATT_SPD,
	MOVE_SPD,
};

const char* StatTypeToString(STAT_TYPE stat);

// Used to distinguish between different types of damage sources,
// allowing mitigation/buffs to apply selectively (e.g. only physical).
enum class DAMAGE_TYPE {
	PHYSICAL,
	MAGICAL,
	ELEMENTAL,
	TRUE_DAMAGE
};

#endif