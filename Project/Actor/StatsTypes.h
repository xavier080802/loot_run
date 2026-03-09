#ifndef _STATS_TYPES_H_
#define _STATS_TYPES_H_

//TODO: make map instead. key - STAT_TYPE
/**
 * @brief The core numbers that describe how strong, fast, and tanky a character is.
 *
 * Every actor (Player AND enemies) has one of these. The values here are the real,
 * final numbers used in gameplay every frame after gear and upgrades are applied.
 */
struct ActorStats
{
    float maxHP = 0.0f; // The highest amount of HP this character can ever have.
    float attack = 0.0f; // How hard they hit. Used as base damage in combat calculations.
    float defense = 0.0f; // Reduces incoming physical/magical damage. More = less damage taken.
    float moveSpeed = 0.0f; // How fast the character moves around, measured in pixels per second.
    float attackSpeed = 0.0f; // How many times per second the character can attack.
};

/**
 * @brief The flat stat bonuses provided by equipped gear.
 *
 * When the player wears a helmet that gives +10 defense, that +10 lives here.
 * These are purely additive (no multiplication yet).
 * Collected by Inventory::GetEquipmentModifiers() and fed into StatsCalc::ComputeFinalStats().
 */
struct EquipmentModifiers
{
    ActorStats additive; // The total sum of all additive stat bonuses from all worn gear.
};

/**
 * @brief Percentage multipliers applied to stats after equipment bonuses are added.
 *
 * Think of these as power-up tiers: a 1.5 hpMult means the player's HP is boosted by 50%.
 * Currently used for upgrade systems (e.g. a well/altar that permanently boosts a stat).
 */
struct UpgradeMultipliers
{
	float hpMult = 1.0f; // Multiplier for max HP. 1.0 = no change, 2.0 = double HP.
	float attackMult = 1.0f; // Multiplier for attack damage.
	float defenseMult = 1.0f; // Multiplier for defense.
	float moveSpeedMult = 1.0f; // Multiplier for movement speed from the shop.
	float attackSpeedMult = 1.0f; // Multiplier for how fast the character attacks.
};

/**
 * @brief Tags that identify which specific stat a status effect is targeting.
 *
 * Status effects (like a burn or a slow) reference this enum to know
 * which number in ActorStats they should be modifying.
 */
enum STAT_TYPE {
    MAX_HP, // Affects maximum health points.
    DEF, // Affects defense / damage reduction.
    ATT, // Affects physical attack power.
    ATT_SPD, // Affects how fast the character attacks.
    MOVE_SPD, // Affects how fast the character moves.
};

const char* StatTypeToString(STAT_TYPE stat);

/**
 * @brief Identifies what kind of hit a damage event is, so defense can be applied correctly.
 *
 * Physical and Magical hits are reduced by defense.
 * Elemental and True Damage bypass defense entirely.
 * This matters because some items or enemies might have resistances against specific types.
 */
enum class DAMAGE_TYPE {
    PHYSICAL, // Normal sword/fist attacks. Defense reduces this.
    MAGICAL, // Spell-type attacks. Defense reduces this too.
    ELEMENTAL, // Fire, ice, lightning etc. Bypasses normal defense.
    TRUE_DAMAGE // Always deals the full listed damage. Nothing can reduce it.
};

#endif