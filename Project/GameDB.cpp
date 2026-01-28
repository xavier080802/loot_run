#include "GameDB.h"
#include <vector>

namespace
{
	std::vector<EquipmentData>& EquipmentRegistry() {
		static std::vector<EquipmentData> items =
		{
			// Demo Sword: +10 attack
			{
				1,
				EquipSlot::Weapon,
				WeaponType::Sword,
				AttackType::SwingArc,
				false,
				EquipmentModifiers{ ActorStats{ 0.0f, 10.0f, 0.0f, 0.0f, 0.0f } },
				"Demo Sword"
			},

			// Demo Armor: +25 HP, +5 defense
			{
				2,
				EquipSlot::Armor,
				WeaponType::None,
				AttackType::None,
				false,
				EquipmentModifiers{ ActorStats{ 25.0f, 0.0f, 5.0f, 0.0f, 0.0f } },
				"Demo Armor"
			},
		};

		return items;
	}
}

namespace GameDB
{
	const EquipmentData* GetEquipmentData(int id) {
		auto& items = EquipmentRegistry();
		for (auto& it : items)
			if (it.id == id) return &it;
		return nullptr;
	}
}