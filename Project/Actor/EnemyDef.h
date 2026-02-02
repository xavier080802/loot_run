#pragma once
#include "StatsTypes.h"
#include "AEEngine.h"
#include "../Helpers/ColorUtils.h"
#include <string>

enum class EnemyMesh : unsigned char
{
	Circle = 0,
	Square = 1,
};

struct EnemyRenderDef
{
	float radius = 15.0f;
	EnemyMesh mesh = EnemyMesh::Circle;
	Color tint = { 255, 255, 255, 255 };
};

struct EnemyDef
{
	int id = 0;

	//If we want to display name
	std::string name;

	ActorStats baseStats{};
	int dropTableId = 0;

	EnemyRenderDef render{};
};
