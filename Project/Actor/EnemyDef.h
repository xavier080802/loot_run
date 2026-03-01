#pragma once
#include "StatsTypes.h"
#include "AEEngine.h"
#include "../Helpers/ColorUtils.h"
#include "../Inventory/EquipmentTypes.h"
#include <string>

// Represents the shape used to render the enemy.
enum class EnemyMesh : unsigned char
{
	Circle = 0,
	Square = 1,
};

// Contains visual rendering parameters for an enemy type.
struct EnemyRenderDef
{
	float radius = 15.0f; // Size of the mesh
	EnemyMesh mesh = EnemyMesh::Circle; // Shape of the mesh
	Color tint = { 255, 0, 0, 255 }; // RGBA color over the mesh (Red by default)
	std::string texturePath = ""; // Path to sprite texture. If empty, uses colored mesh instead.
};

// Defines the attack pattern capabilities of an enemy
struct EnemyAttackDef
{
	float range = 100.0f;          // Range required to begin attacking
	float cooldown = 2.0f;         // Delay in seconds between attacks

	// Mock weapon used to route through Combat::ExecuteAttack
	EquipmentData mockWeapon{};
};

// Core data structure defining a specific type of enemy.
// This is used as a blueprint to initialize runtime Enemy instances.
struct EnemyDef
{
	int id = 0; // Unique identifier for the enemy type.

	//If we want to display name
	std::string name;

	ActorStats baseStats{}; // The foundational stats given to this enemy type on spawn.
	int dropTableId = 0; // ID referencing the loot table this enemy pulls from upon death.

	EnemyRenderDef render{}; // Visual configuration for the enemy.
	EnemyAttackDef attack{}; // Attack pattern configuration.
};
