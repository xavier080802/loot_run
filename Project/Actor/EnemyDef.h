#pragma once
#include "StatsTypes.h"
#include "AEEngine.h"
#include "../Helpers/ColorUtils.h"
#include "../Inventory/EquipmentTypes.h"
#include <string>

/**
 * @brief Describes what shape to draw for this enemy.
 *
 * Either a filled circle or a square. Loaded in from enemies.json and directly
 * controls how the enemy appears rendered in the game world.
 */
enum class ENEMY_MESH : unsigned char
{
    CIRCLE = 0, // Enemy is drawn as a circle (used for the Slime and most AI).
    SQUARE = 1, // Enemy is drawn as a square.
};

/**
 * @brief Classifies an enemy's tier / difficulty category.
 *
 * Loaded from the "category" field in enemies.json.
 * Used by GameDB spawn-helper functions to filter the registry by tier.
 */
enum class ENEMY_CATEGORY : unsigned char
{
    NORMAL = 0, // Standard weak enemy, encountered often.
    ELITE  = 1, // Stronger variant with higher stats.
    BOSS   = 2, // Unique powerful enemy, typically one per area.
};

/**
 * @brief Holds the visual rendering information for how an enemy looks in game.
 *
 * Every enemy type needs to know how big to draw itself and what it looks like.
 * These values come from the "render" section of each entry in enemies.json.
 */
struct EnemyRenderDef
{
    float radius = 15.0f;           // The pixel size/radius of the rendered mesh.
    ENEMY_MESH mesh = ENEMY_MESH::CIRCLE; // Circle or Square. Affects render shape + collision.
    Color tint = { 255, 0, 0, 255 };    // Tint color applied on top of the mesh (Red by default).
    std::string texturePath = "";   // If not empty, draws this image instead of a colored mesh.
};

/**
 * @brief Defines how and when an enemy attacks.
 *
 * Loaded from the "attack" block in enemies.json for each enemy type.
 * The mockWeapon is interesting instead of building a separate attack system for enemies,
 * enemies reuse the exact same EquipmentData struct as the player's weapons.
 * This means Combat::ExecuteAttack() works the same for both player and enemies!
 */
struct EnemyAttackDef
{
    float aggroRange = 500.0f; // How close player must be for enemy to wake up and chase.
    float range = 100.0f;     // How close the enemy has to get before it stops chasing and starts attacking.
    float cooldown = 2.0f;    // How many seconds the enemy waits between each attack swing.

    // The weapon parameters this enemy uses. Routes through Combat::ExecuteAttack() so
    // it can fire projectiles, create melee hitboxes etc. The same way the player does.
    EquipmentData mockWeapon{};
};

/**
 * @brief The master blueprint for a specific enemy type (like Slime or Boss).
 *
 * This is a DATA-ONLY struct loaded once from enemies.json when the game starts.
 * It never gets modified at runtime -- it just lives in the GameDB as a static reference.
 * When an Enemy is spawned, it's given a pointer to its matching EnemyDef
 * and uses it to set up its health, appearance, and attack pattern.
 *
 * @note Loaded by:
 *   - GameDB::LoadEnemyDefs() - parses enemies.json on game startup.
 * @note Referenced by:
 *   - Enemy::InitEnemyRuntime() - reads the EnemyDef to configure a live Enemy instance.
 */
struct EnemyDef
{
    int id = 0; // A unique number used to look up this enemy type from GameDB.
    std::string name; // The display name of the enemy (e.g. "Slime", "Boss").
    ENEMY_CATEGORY category = ENEMY_CATEGORY::NORMAL; // Tier classification: Normal, Elite, or Boss.
    ActorStats baseStats{}; // The starting stats for this type (health, speed, damage etc.).
    int dropTableId = 0; // Which loot table to roll when this enemy dies.
    EnemyRenderDef render{}; // How this enemy looks when drawn on screen.
    EnemyAttackDef attack{}; // How and when this enemy attacks.
};
