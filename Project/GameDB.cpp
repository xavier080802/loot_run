#include "GameDB.h"
#include <vector>
#include <fstream>
#include <json/json.h>
#include <iostream>
#include "./Helpers/ColorUtils.h"

namespace
{
	static EquipmentData MakeWeapon(
		int id,
		const char* name,
		WeaponType weaponType,
		AttackType attackType,
		bool isRanged,
		Rarity rarity,
		const ActorStats& additiveStats)
	{
		EquipmentData e{};
		e.id = id;
		e.slot = EquipSlot::Weapon;
		e.armorSlot = ArmorSlot::None;
		e.weaponType = weaponType;
		e.attackType = attackType;
		e.isRanged = isRanged;
		e.rarity = rarity;
		e.mods.additive = additiveStats;
		e.name = name;
		return e;
	}

	static EquipmentData MakeArmor(
		int id,
		const char* name,
		ArmorSlot armorSlot,
		Rarity rarity,
		const ActorStats& additiveStats)
	{
		EquipmentData e{};
		e.id = id;
		e.slot = EquipSlot::Armor;
		e.armorSlot = armorSlot;
		e.weaponType = WeaponType::None;
		e.attackType = AttackType::None;
		e.isRanged = false;
		e.rarity = rarity;
		e.mods.additive = additiveStats;
		e.name = name;
		return e;
	}

	std::vector<EquipmentData>& EquipmentRegistry()
	{
		static std::vector<EquipmentData> items =
		{
			// Demo Sword: +10 attack
			MakeWeapon(
				1,
				"Demo Sword",
				WeaponType::Sword,
				AttackType::SwingArc,
				false,
				Rarity::Common,
				ActorStats{ 0.0f, 10.0f, 0.0f, 0.0f, 0.0f }
			),

			// Demo Armor (Body): +25 HP, +5 defense
			MakeArmor(
				2,
				"Demo Armor",
				ArmorSlot::Body,
				Rarity::Common,
				ActorStats{ 25.0f, 0.0f, 5.0f, 0.0f, 0.0f }
			),
				// Demo Bow: +5 attack
			MakeWeapon(
				3,
				"Demo Bow",
				WeaponType::Bow,
				AttackType::Projectile,
				true,
				Rarity::Common,
				ActorStats{ 0.0f, 5.0f, 0.0f, 0.0f, 0.0f }
			),
		};

		return items;
	}

	std::vector<EnemyDef>& EnemyRegistry()
	{
		static std::vector<EnemyDef> enemies;
		return enemies;
	}

	static bool ReadTintRGBA(const Json::Value& tintArr, Color& out)
	{
		if (!tintArr.isArray() || tintArr.size() != 4) return false;

		int r = tintArr[0].asInt();
		int g = tintArr[1].asInt();
		int b = tintArr[2].asInt();
		int a = tintArr[3].asInt();

		// Clamp to [0 - 255] incase someone puts wrong values
		if (r < 0) r = 0; if (r > 255) r = 255;
		if (g < 0) g = 0; if (g > 255) g = 255;
		if (b < 0) b = 0; if (b > 255) b = 255;
		if (a < 0) a = 0; if (a > 255) a = 255;

		out = CreateColor((u8)r, (u8)g, (u8)b, (u8)a);
		return true;
	}

	static EnemyMesh ParseEnemyMesh(const Json::Value& meshVal)
	{
		if (!meshVal.isString()) return EnemyMesh::Circle;

		std::string m = meshVal.asString();
		if (m == "square" || m == "SQUARE") return EnemyMesh::Square;
		return EnemyMesh::Circle; // default
	}

}

namespace GameDB
{
	const EquipmentData* GetEquipmentData(int id)
	{
		auto& items = EquipmentRegistry();
		for (auto& it : items)
		{
			if (it.id == id)
				return &it;
		}
		return nullptr;
	}

	bool LoadEnemyDefs(const char* path)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open()) { return false; }

		//debugging
		file.seekg(0, std::ios::end);
		std::streamoff size = file.tellg();
		std::cout << "LoadEnemyDefs: bytes=" << (long long)size << "\n";

		if (size <= 0)
		{
			std::cout << "LoadEnemyDefs: file is EMPTY (wrong path or empty file)\n";
			return false;
		}

		//rewinding for json parser
		file.clear();
		file.seekg(0, std::ios::beg);

		Json::Value root;
		Json::CharReaderBuilder builder;
		std::string errs;

		if (!Json::parseFromStream(builder, file, &root, &errs))
		{
			std::cout << "LoadEnemyDefs: parse failed: " << errs << "\n";
			return false;
		}

		if (!root.isObject() || !root.isMember("enemies") || !root["enemies"].isArray()) {
			std::cout << "LoadEnemyDefs: missing/invalid 'enemies' array\n";
			return false;
		}
		auto& reg = EnemyRegistry();
		reg.clear();

		for (const auto& e : root["enemies"])
		{
			EnemyDef def{};

			def.id = e.get("id", 0).asInt();
			def.name = e.get("name", "").asString();
			def.dropTableId = e.get("dropTableId", 0).asInt();

			//base stats
			const auto& s = e["baseStats"];
			def.baseStats.maxHP = s.get("maxHP", 0.0f).asFloat();
			def.baseStats.attack = s.get("attack", 0.0f).asFloat();
			def.baseStats.defense = s.get("defense", 0.0f).asFloat();
			def.baseStats.moveSpeed = s.get("moveSpeed", 0.0f).asFloat();
			def.baseStats.attackSpeed = s.get("attackSpeed", 1.0f).asFloat();

			//render info
			const auto& r = e["render"];
			def.render.radius = r.get("radius", def.render.radius).asFloat();
			def.render.mesh = ParseEnemyMesh(r.get("mesh", "circle"));

			Color tint = def.render.tint;
			ReadTintRGBA(r["tint"], tint);// if missing or invalid keep it default
			def.render.tint = tint;

			reg.push_back(def);
		}
		std::cout << "LoadEnemyDefs: loaded " << (int)reg.size() << " enemies from " << path << "\n";
		return true;
	}

	const EnemyDef* GetEnemyDef(int id)
	{
		for (const auto& e : EnemyRegistry())
			if (e.id == id)
				return &e;
		return nullptr;
	}
}
