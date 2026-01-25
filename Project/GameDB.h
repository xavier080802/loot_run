#pragma once
#include "./Drops/DropTypes.h"
#include "./Inventory/EquipmentTypes.h"

struct EnemyDef; // forward declared if you want it here

namespace GameDB
{
	const DropTable* GetDropTable(int id);
	const EquipmentData* GetEquipmentData(int id);
}