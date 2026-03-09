#include "GameDB.h"
#include <vector>
#include <fstream>
#include <json/json.h>
#include <iostream>
#include <cstdlib>
#include "./Helpers/ColorUtils.h"

namespace
{
    static std::vector<EnemyDef> sEnemyRegistry;
    static std::vector<EquipmentData> sEquipmentRegistry;
    static std::vector<DropTable> sDropTables;
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
    /**
     * @brief Retrieves a drop table by its unique ID.
     *
     * Scans the loaded drop tables array and returns a pointer to the matching table.
     *
     * @param id  The unique identifier of the drop table to find. Passed by VALUE.
     *
     * @return A CONST POINTER to the DropTable if found, nullptr otherwise.
     *
     * @note Called by:
     *   - DropSystem - when resolving a drop table ID into a DropTable reference.
     */
    const DropTable* GetDropTable(int id)
    {
        for (const auto& t : sDropTables)
        {
            if (t.id == id) return &t;
        }
        return nullptr;
    }

    bool LoadDropTables(const char* path)
    {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cout << "LoadDropTables: Cannot open " << path << "\n";
            return false;
        }

        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(file, root, false)) {
            std::cout << "LoadDropTables error parsing " << path << ":\n" << reader.getFormattedErrorMessages() << "\n";
            return false;
        }

        if (!root.isMember("dropTables") || !root["dropTables"].isArray()) {
            std::cout << "LoadDropTables: Missing 'dropTables' array.\n";
            return false;
        }

        // Expected JSON structure:
        /*
        "dropTables": [
            {
              "id": 0,
              "entries": [
                {
                  "type": "Coin",
                  "chance": 0.5,
                  "minAmount": 5,
                  "maxAmount": 10
                }
              ]
            }
        ]
        */
        sDropTables.clear();
        for (const auto& dtJson : root["dropTables"])
        {
            DropTable dt{};
            dt.id = dtJson.get("id", 0).asInt(); // Register the unique ID for this specific drop table
            dt.entryCount = 0;

            if (dtJson.isMember("entries") && dtJson["entries"].isArray())
            {
                // Loop through all potential items in this table and parse their drop logic
                for (const auto& entryJson : dtJson["entries"])
                {
                    if (dt.entryCount >= DropTable::MAX_ENTRIES) break;

                    DropEntry e{};
                    std::string typeStr = entryJson.get("type", "Coin").asString();
                    
                    // Map the string from JSON to our C++ DropType enum
                    if (typeStr == "Coin") e.type = DropType::Coin;
                    else if (typeStr == "Ammo") e.type = DropType::Ammo;
                    else if (typeStr == "Equipment") e.type = DropType::Equipment;
                    else if (typeStr == "Heal") e.type = DropType::Heal;
                    else if (typeStr == "Buff") e.type = DropType::Buff;
                    else if (typeStr == "None") e.type = DropType::None;
                    
                    // Only fully parse Equipment specifics if this drop guarantees equipment
                    if (e.type == DropType::Equipment) {
                        std::string catStr = entryJson.get("category", "Melee").asString();
                        if (catStr == "Melee") e.equipmentCategory = EquipmentCategory::Melee;
                        else if (catStr == "Ranged") e.equipmentCategory = EquipmentCategory::Ranged;
                        else if (catStr == "Head") e.equipmentCategory = EquipmentCategory::Head;
                        else if (catStr == "Body") e.equipmentCategory = EquipmentCategory::Body;
                        else if (catStr == "Hands") e.equipmentCategory = EquipmentCategory::Hands;
                        else if (catStr == "Feet") e.equipmentCategory = EquipmentCategory::Feet;
                        else e.equipmentCategory = EquipmentCategory::None; // Parses "Any" or unrecognized categories as a wildcard
                        
                        e.itemId = entryJson.get("itemId", 0).asInt();
                    }

                    // Store the probabilities and quantities
                    e.chance = entryJson.get("chance", 1.0f).asFloat();
                    e.minAmount = entryJson.get("minAmount", 1).asInt();
                    e.maxAmount = entryJson.get("maxAmount", 1).asInt();
                    
                    dt.entries[dt.entryCount++] = e; // Push the constructed entry into the array
                }
            }
            sDropTables.push_back(dt); // Register the fully parsed table
        }

        std::cout << "LoadDropTables: loaded " << sDropTables.size() << " drop tables from " << path << "\n";
        file.close();
        return true;
    }


	/**
	 * @brief Parses JSON file to load Enemy Definitions into the EnemyRegistry collection.
	 *
	 * Makes enemy variations data-driven rather than hardcoded. Pulls in stats, render data,
	 * and attack patterns for each enemy type.
	 *
	 * @param path  File path to the enemies JSON file. Passed as a CONST POINTER.
     *
     * @return true if successfully loaded and parsed, false otherwise.
     *
     * @note Called by:
     *   - GameState::Init() - once on game startup.
	 */
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

			// Parse category (Normal / Elite / Boss)
			std::string catStr = e.get("category", "Normal").asString();
			if (catStr == "Elite"  || catStr == "ELITE")  def.category = EnemyCategory::Elite;
			else if (catStr == "Boss" || catStr == "BOSS") def.category = EnemyCategory::Boss;
			else                                           def.category = EnemyCategory::Normal;

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
				def.attack.aggroRange = atk.get("aggroRange", def.attack.aggroRange).asFloat();
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

    /**
     * @brief Loads equipment definitions from a JSON file into the registry.
     *
     * Reads a list of equipment items (weapons, armor, bows) and adds them to
     * the global EquipmentRegistry. Can be called multiple times for different item categories.
     *
     * @param path      File path to the equipment JSON file. Passed as a CONST POINTER.
     * @param category  The EquipmentCategory classification for these loaded items. Passed by VALUE.
     *
     * @return true if successfully loaded and parsed, false otherwise.
     *
     * @note Called by:
     *   - GameState::Init() - called multiple times for each equipment JSON file.
     */
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
            e.texturePath = _strdup(item.get("texturePath", "").asString().c_str());
            
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
            else if (rarityStr == "Mythical") e.rarity = Rarity::Mythical;
            else e.rarity = Rarity::Common;

            e.sellPrice = item.get("sellPrice", 10).asInt();

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

    /**
     * @brief Loads the player's base stats from a JSON file.
     *
     * Sets the default, starting stats for the player character before any equipment
     * or shop upgrades are applied.
     *
     * @param path  File path to the player definition JSON file. Passed as a CONST POINTER.
     *
     * @return true if successfully loaded and parsed, false otherwise.
     *
     * @note Called by:
     *   - GameState::Init() - once on game startup.
     */
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

    /**
     * @brief Loads the player's starter inventory setup from a JSON file.
     *
     * Determines which equipment IDs the player starts the game with.
     *
     * @param path  File path to the inventory JSON file. Passed as a CONST POINTER.
     *
     * @return true if successfully loaded and parsed, false otherwise.
     *
     * @note Called by:
     *   - GameState::Init() - once on game startup.
     */
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
        sPlayerInventory.ammo = root.get("ammo", 0).asInt();

        std::cout << "LoadPlayerInventory: Successfully loaded player inventory from " << path << "\n";
        file.close();
        return true;
    }

    /**
     * @brief Gets the player's base stats loaded from the JSON config.
     *
     * @return A CONST REFERENCE to the player's base ActorStats.
     *
     * @note Called by:
     *   - DropSystem - to get player max hp for heal drops.
     */
    const ActorStats& GetPlayerBaseStats() { return sPlayerBaseStats; }

    /**
     * @brief Gets the player's starter inventory definition.
     *
     * @return A CONST REFERENCE to the PlayerInventoryDef struct.
     *
     * @note Called by:
     *   - Player::InitPlayerRuntime() - to know what starter gear to give.
     */
    const PlayerInventoryDef& GetPlayerStarterInventory() { return sPlayerInventory; }

	/**
	 * @brief Searches the equipment registry for a specific item by category and ID.
	 *
	 * Used to retrieve an item's data when loading starting gear or rolling a specific drop.
	 *
	 * @param category  The EquipmentCategory to filter by. Passed by VALUE.
	 * @param id        The unique numeric ID of the equipment to find. Passed by VALUE.
	 *
	 * @return A CONST POINTER to the EquipmentData if found, nullptr otherwise.
     *
     * @note Called by:
     *   - Player::InitPlayerRuntime() - to load player starter equipment.
     *   - DropSystem - when resolving a drop table entry into an equipment drop.
	 */
	const EquipmentData* GetEquipmentData(EquipmentCategory category, int id)
	{
		for (const auto& e : EquipmentRegistry())
			if (e.id == id && e.category == category)
				return &e;
		return nullptr;
	}

	int GetRarityWeight(Rarity r)
	{
		switch (r)
		{
		case Rarity::Common: return 100;
		case Rarity::Uncommon: return 50;
		case Rarity::Rare: return 20;
		case Rarity::Epic: return 10;
		case Rarity::Legendary: return 4;
		case Rarity::Mythical: return 1;
		default: return 100;
		}
	}

    /**
     * @brief Returns a random piece of equipment from the registry, weighted by rarity.
     *
     * Items with lower rarities (Common) have a much higher chance to be selected
     * than items with higher rarities (Mythical). Used for generating generic loot drops.
     *
     * @return A CONST POINTER to a randomly selected EquipmentData, or nullptr if registry is empty.
     *
     * @note Called by:
     *   - DropSystem - when resolving a wildcard "Any Equipment" drop.
     */
    const EquipmentData* GetRandomEquipment()
    {
        auto& reg = EquipmentRegistry();
        if (reg.empty()) return nullptr;

		int totalWeight = 0;
		for (const auto& item : reg)
			totalWeight += GetRarityWeight(item.rarity);

		if (totalWeight <= 0) return &reg[0];

		int randVal = rand() % totalWeight;
		for (const auto& item : reg)
		{
			randVal -= GetRarityWeight(item.rarity);
			if (randVal < 0)
				return &item;
		}

        return &reg.back();
    }

    /**
     * @brief Frees allocated memory for dynamic strings in the equipment registry.
     *
     * Should be called on application exit to prevent memory leaks from _strdup allocations.
     *
     * @note Called by:
     *   - Application Cleanup / Exit.
     */
    void UnloadEquipmentReg()
    {
        for (EquipmentData& d : sEquipmentRegistry) {
            free((void*)d.name);
            free((void*)d.texturePath);
        }
    }

    /**
     * @brief Retrieves an enemy definition by its unique ID.
     *
     * Used when spawning a specific type of enemy to access its template data.
     *
     * @param id  The unique identifier of the enemy to find. Passed by VALUE.
     *
     * @return A CONST POINTER to the EnemyDef if found, nullptr otherwise.
     *
     * @note Called by:
     *   - GameState::Init() - when spawning Slimes or Bosses.
     */
    const EnemyDef* GetEnemyDef(int id)
    {
        for (const auto& e : EnemyRegistry()) {
            if (e.id == id) { return &e; }
        }
        return nullptr;
    }

    /**
     * @brief Returns a uniformly random EnemyDef whose category is Normal.
     *
     * Builds a filtered view of the registry at call time, then picks a
     * random entry. Returns nullptr if no Normal enemies are registered.
     *
     * @return A CONST POINTER to a randomly selected Normal EnemyDef, or nullptr.
     *
     * @note Called by:
     *   - Any spawner that needs a generic Normal-tier enemy.
     */
    const EnemyDef* GetRandomNormalEnemy()
    {
        std::vector<const EnemyDef*> pool;
        for (const auto& e : EnemyRegistry())
            if (e.category == EnemyCategory::Normal)
                pool.push_back(&e);

        if (pool.empty()) return nullptr;
        return pool[rand() % pool.size()];
    }

    /**
     * @brief Returns a uniformly random EnemyDef whose category is Elite.
     *
     * Builds a filtered view of the registry at call time, then picks a
     * random entry. Returns nullptr if no Elite enemies are registered.
     *
     * @return A CONST POINTER to a randomly selected Elite EnemyDef, or nullptr.
     *
     * @note Called by:
     *   - Any spawner that needs a stronger Elite-tier enemy.
     */
    const EnemyDef* GetRandomEliteEnemy()
    {
        std::vector<const EnemyDef*> pool;
        for (const auto& e : EnemyRegistry())
            if (e.category == EnemyCategory::Elite)
                pool.push_back(&e);

        if (pool.empty()) return nullptr;
        return pool[rand() % pool.size()];
    }

    /**
     * @brief Returns a uniformly random EnemyDef whose category is Boss.
     *
     * Builds a filtered view of the registry at call time, then picks a
     * random entry. Returns nullptr if no Boss enemies are registered.
     *
     * @return A CONST POINTER to a randomly selected Boss EnemyDef, or nullptr.
     *
     * @note Called by:
     *   - Whichever functions ends up signaling to spawn a Boss.
     */
    const EnemyDef* GetRandomBossEnemy()
    {
        std::vector<const EnemyDef*> pool;
        for (const auto& e : EnemyRegistry())
            if (e.category == EnemyCategory::Boss)
                pool.push_back(&e);

        if (pool.empty()) return nullptr;
        return pool[rand() % pool.size()];
    }

    /**
     * @brief Picks a random Normal or Elite EnemyDef using weighted probability.
     *
     * Generates a float in [0, 1) and uses cumulative probability bands to
     * decide the category. Boss enemies are intentionally excluded as they have
     * separate, hand-crafted spawn requirements.
     *
     * Default weights: 70% Normal, 30% Elite (sum = 1.0).
     * If the roll somehow falls outside both bands, nullptr is returned.
     *
     * @param normalChance  Probability [0..1] of selecting a Normal enemy.
     * @param eliteChance   Probability [0..1] of selecting an Elite enemy.
     *
     * @return A CONST POINTER to a randomly selected Normal or Elite EnemyDef,
     *         or nullptr if the pool for the chosen tier is empty.
     *
     * @note Called by:
     *   - Generic wave spawners that want tier-weighted variety without Bosses.
     */
    const EnemyDef* GetWeightedRandomEnemy(float normalChance, float eliteChance)
    {
        // Generate a value in [0.0, 1.0)
        float roll = static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) + 1.0f);

        if (roll < normalChance)
            return GetRandomNormalEnemy();

        if (roll < normalChance + eliteChance)
            return GetRandomEliteEnemy();

        // Roll fell outside both bands; Boss enemies require explicit spawning.
        return nullptr;
    }
}
