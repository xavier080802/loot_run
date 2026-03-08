#include "GameDB.h"
#include <vector>
#include <fstream>
#include <json/json.h>
#include <iostream>
#include "./Helpers/ColorUtils.h"

namespace
{
    static std::vector<EnemyDef> sEnemyRegistry;
    static std::vector<EquipmentData> sEquipmentRegistry;
    static ActorStats sPlayerBaseStats;
    static GameDB::PlayerInventoryDef sPlayerInventory;
    static std::vector<int> sPlayerStarterEquipmentIds;

	std::vector<EquipmentData>& EquipmentRegistry()
	{
		return sEquipmentRegistry;
	}

	std::vector<EnemyDef>& EnemyRegistry()
	{
		return sEnemyRegistry;
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


	// Parses JSON file to load Enemy Definitions into the EnemyRegistry collection.
	// Makes enemy variations data-driven rather than hardcoded.
	bool LoadEnemyDefs(const char* path)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open()) { return false; }

		//debugging size of file
		file.seekg(0, std::ios::end);
		std::streamoff size = file.tellg();
		std::cout << "LoadEnemyDefs: bytes=" << (long long)size << "\n";

		if (size <= 0)
		{
			std::cout << "LoadEnemyDefs: file is EMPTY (wrong path or empty file)\n";
			return false;
		}

		//its rewind time
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

			// Extract base identity data
			def.id = e.get("id", 0).asInt();
			def.name = e.get("name", "").asString();
			def.dropTableId = e.get("dropTableId", 0).asInt();

			// Parse base stats
			const auto& s = e["baseStats"];
			def.baseStats.maxHP = s.get("maxHP", 0.0f).asFloat();
			def.baseStats.attack = s.get("attack", 0.0f).asFloat();
			def.baseStats.defense = s.get("defense", 0.0f).asFloat();
			def.baseStats.moveSpeed = s.get("moveSpeed", 0.0f).asFloat();
			def.baseStats.attackSpeed = s.get("attackSpeed", 1.0f).asFloat();

			// Parse render info to configure the visual representation
			const auto& r = e["render"];
			def.render.radius = r.get("radius", def.render.radius).asFloat();
			def.render.mesh = ParseEnemyMesh(r.get("mesh", "circle"));
			def.render.texturePath = r.get("texturePath", "").asString();

			Color tint = def.render.tint;
			ReadTintRGBA(r["tint"], tint);// if missing or invalid keep it default
			def.render.tint = tint;

			// Parse Enemy Attack pattern
			if (e.isMember("attack")) {
				const auto& atk = e["attack"];
				def.attack.range = atk.get("range", def.attack.range).asFloat();
				def.attack.cooldown = atk.get("cooldown", def.attack.cooldown).asFloat();

				// Build the mock weapon for Combat::ExecuteAttack
				std::string atkTypeStr = atk.get("attackType", "SwingArc").asString();
				if (atkTypeStr == "Projectile") {
					def.attack.mockWeapon.attackType = AttackType::Projectile;
					def.attack.mockWeapon.isRanged = true;
				} else if (atkTypeStr == "Stab") {
					def.attack.mockWeapon.attackType = AttackType::Stab;
					def.attack.mockWeapon.isRanged = false;
				} else if (atkTypeStr == "CircleAOE") {
					def.attack.mockWeapon.attackType = AttackType::CircleAOE;
					def.attack.mockWeapon.isRanged = false;
				} else {
					def.attack.mockWeapon.attackType = AttackType::SwingArc;
					def.attack.mockWeapon.isRanged = false;
				}

				std::string eleStr = atk.get("element", "NONE").asString();
				if (eleStr == "BLOOD") def.attack.mockWeapon.element = Elements::ELEMENT_TYPE::BLOOD;
				else if (eleStr == "SUN") def.attack.mockWeapon.element = Elements::ELEMENT_TYPE::SUN;
				else if (eleStr == "MOON") def.attack.mockWeapon.element = Elements::ELEMENT_TYPE::MOON;
				else def.attack.mockWeapon.element = Elements::ELEMENT_TYPE::NONE;

                def.attack.mockWeapon.knockback = atk.get("knockback", 100.0f).asFloat();
                def.attack.mockWeapon.attackSize = atk.get("attackSize", 10.0f).asFloat();
			}

			// Add fully constructed definition to registry
			reg.push_back(def);
		}
		std::cout << "LoadEnemyDefs: loaded " << (int)reg.size() << " enemies from " << path << "\n";
        file.close();
		return true;
	}

    bool LoadEquipmentDefs(const char* path, EquipmentCategory category)
    {
        auto& reg = EquipmentRegistry();

        std::ifstream file(path);
        if (!file.is_open()) {
            std::cout << "LoadEquipmentDefs: Cannot open " << path << "\n";
            return false;
        }

        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(file, root, false)) {
            std::cout << "LoadEquipmentDefs error parsing " << path << ":\n" << reader.getFormattedErrorMessages() << "\n";
            return false;
        }
        
        if (!root.isArray()) {
            std::cout << "LoadEquipmentDefs data in " << path << " is not a JSON array.\n";
            return false;
        }

        int totalLoaded = 0;
        for (auto& item : root) {
            EquipmentData e{};
            e.id = item.get("id", 0).asInt();
            e.category = category;
            e.name = _strdup(item.get("name", "Unknown").asString().c_str());
            
            std::string slotStr = item.get("slot", "Weapon").asString();
            e.slot = (slotStr == "Armor") ? EquipSlot::Armor : EquipSlot::Weapon;
            
            std::string armorSlotStr = item.get("armorSlot", "None").asString();
            if (armorSlotStr == "Head") e.armorSlot = ArmorSlot::Head;
            else if (armorSlotStr == "Body") e.armorSlot = ArmorSlot::Body;
            else if (armorSlotStr == "Hands") e.armorSlot = ArmorSlot::Hands;
            else if (armorSlotStr == "Feet") e.armorSlot = ArmorSlot::Feet;
            else e.armorSlot = ArmorSlot::None;
            
            std::string wTypeStr = item.get("weaponType", "None").asString();
            if (wTypeStr == "Sword") e.weaponType = WeaponType::Sword;
            else if (wTypeStr == "Bow") e.weaponType = WeaponType::Bow;
            else e.weaponType = WeaponType::None;

            std::string aTypeStr = item.get("attackType", "None").asString();
            if (aTypeStr == "SwingArc") e.attackType = AttackType::SwingArc;
            else if (aTypeStr == "Projectile") e.attackType = AttackType::Projectile;
            else if (aTypeStr == "CircleAOE") e.attackType = AttackType::CircleAOE;
            else if (aTypeStr == "Stab") e.attackType = AttackType::Stab;
            else e.attackType = AttackType::None;

            e.isRanged = item.get("isRanged", false).asBool();

            std::string eleStr = item.get("element", "NONE").asString();
            if (eleStr == "BLOOD") e.element = Elements::ELEMENT_TYPE::BLOOD;
            else if (eleStr == "SUN") e.element = Elements::ELEMENT_TYPE::SUN;
            else if (eleStr == "MOON") e.element = Elements::ELEMENT_TYPE::MOON;
            else e.element = Elements::ELEMENT_TYPE::NONE;

            e.knockback = item.get("knockback", 100.0f).asFloat();
            e.attackSize = item.get("attackSize", 10.0f).asFloat();

            std::string rarityStr = item.get("rarity", "Common").asString();
            if (rarityStr == "Uncommon") e.rarity = Rarity::Uncommon;
            else if (rarityStr == "Rare") e.rarity = Rarity::Rare;
            else if (rarityStr == "Epic") e.rarity = Rarity::Epic;
            else if (rarityStr == "Legendary") e.rarity = Rarity::Legendary;
            else e.rarity = Rarity::Common;

            if (item.isMember("stats")) {
                const auto& s = item["stats"];
                e.mods.additive.maxHP       = s.get("maxHP", 0.0f).asFloat();
                e.mods.additive.attack      = s.get("attack", 0.0f).asFloat();
                e.mods.additive.defense     = s.get("defense", 0.0f).asFloat();
                e.mods.additive.moveSpeed   = s.get("moveSpeed", 0.0f).asFloat();
                e.mods.additive.attackSpeed = s.get("attackSpeed", 0.0f).asFloat();
            }

            reg.push_back(e);
            totalLoaded++;
        }
        
        std::cout << "LoadEquipmentDefs: loaded " << totalLoaded << " equipment pieces from " << path << "\n";
        file.close();
        return true;
    }

    bool LoadPlayerDef(const char* path)
    {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cout << "LoadPlayerDef: Cannot open " << path << "\n";
            return false;
        }

        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(file, root, false)) {
            std::cout << "LoadPlayerDef error parsing " << path << ":\n" << reader.getFormattedErrorMessages() << "\n";
            return false;
        }

        if (root.isMember("baseStats")) {
            const auto& s = root["baseStats"];
            sPlayerBaseStats.maxHP = s.get("maxHP", 100.0f).asFloat();
            sPlayerBaseStats.attack = s.get("attack", 10.0f).asFloat();
            sPlayerBaseStats.defense = s.get("defense", 0.0f).asFloat();
            sPlayerBaseStats.moveSpeed = s.get("moveSpeed", 300.0f).asFloat();
            sPlayerBaseStats.attackSpeed = s.get("attackSpeed", 1.0f).asFloat();
        }

        std::cout << "LoadPlayerDef: Successfully loaded player defaults from " << path << "\n";
        file.close();
        return true;
    }

    bool LoadPlayerInventory(const char* path)
    {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cout << "LoadPlayerInventory: Cannot open " << path << "\n";
            return false;
        }

        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(file, root, false)) {
            std::cout << "LoadPlayerInventory error parsing " << path << ":\n" << reader.getFormattedErrorMessages() << "\n";
            return false;
        }

        sPlayerInventory.weapon1 = root.get("weapon1", 0).asInt();
        sPlayerInventory.weapon2 = root.get("weapon2", 0).asInt();
        sPlayerInventory.bow = root.get("bow", 0).asInt();
        sPlayerInventory.head = root.get("head", 0).asInt();
        sPlayerInventory.body = root.get("body", 0).asInt();
        sPlayerInventory.hands = root.get("hands", 0).asInt();
        sPlayerInventory.feet = root.get("feet", 0).asInt();

        std::cout << "LoadPlayerInventory: Successfully loaded player inventory from " << path << "\n";
        file.close();
        return true;
    }

    const ActorStats& GetPlayerBaseStats() { return sPlayerBaseStats; }
    const PlayerInventoryDef& GetPlayerStarterInventory() { return sPlayerInventory; }

	const EquipmentData* GetEquipmentData(EquipmentCategory category, int id)
	{
		for (const auto& e : EquipmentRegistry())
			if (e.id == id && e.category == category)
				return &e;
		return nullptr;
	}

    void UnloadEquipmentReg()
    {
        for (EquipmentData& d : sEquipmentRegistry) {
            free((void*)d.name);
        }
    }

	const EnemyDef* GetEnemyDef(int id)
	{
		for (const auto& e : EnemyRegistry())
			if (e.id == id)
				return &e;
		return nullptr;
	}
}
