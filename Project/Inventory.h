#pragma once
#include <vector>
#include "EquipmentTypes.h"
#include "PickupPayload.h"

class Inventory
{
public:
    void AddEquipment(const EquipmentData* data);
    void AddAmmo(int amount);

    EquipmentModifiers GetEquipmentModifiers() const;
    UpgradeMultipliers GetUpgradeMultipliers() const;

private:
    std::vector<const EquipmentData*> equipment;
    int ammo = 0;
};
