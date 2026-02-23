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
	float hpMult = 1.0f;
	float attackMult = 1.0f;
	float defenseMult = 1.0f;
};

enum STAT_TYPE {
	MAX_HP,
	DEF,
	ATT,
	ATT_SPD,
	MOVE_SPD,
};

// Used to distinguish between different types of damage sources,
// allowing mitigation/buffs to apply selectively (e.g. only physical).
enum class DAMAGE_TYPE {
	PHYSICAL,
	MAGICAL,
	ELEMENTAL,
	TRUE_DAMAGE
};

#endif