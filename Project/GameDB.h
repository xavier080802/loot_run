#pragma once
#include "./Drops/DropTypes.h"
#include "./Inventory/EquipmentTypes.h"

namespace GameDB
{
    const DropTable* GetDropTable(int id);

    // Equipment lookup
    const EquipmentData* GetEquipmentData(int id);

    // persistent-ish runtime owned state (might not use)
    void InitEquipmentRuntime();
    bool IsOwned(int id);
    void SetOwned(int id, bool owned);
}