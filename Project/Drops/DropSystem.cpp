#include "DropSystem.h"
#include "../GameDB.h"
#include "PickupGO.h"
#include "../RenderingManager.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/CoordUtils.h"
#include <cstdlib>
#include <queue>
#include <json/json.h>
#include <fstream>
#include <iostream>

namespace {
    //-----Loaded from json------

    float timeTillPop{ 3.f };
    float fontSize{ 0.3f };
    Color displayTextCol{ 0,200,50, 255 };
    AEVec2 normDisplayPos{0.975f, -0.975f};
    TextOriginPos textAnchor{ TEXT_LOWER_RIGHT };
    unsigned maxDisplayed{ 5 };

    //-----Display stuff-----

    // Trying Deque
    std::deque<std::string> pickupLogQueue{};
    float timer{}; //Timer for clearing queue

    float Rand01()
    {
        return (float)rand() / (float)RAND_MAX;
    }

    int RandRangeInt(int lo, int hi)
    {
        if (hi < lo) { int t = lo; lo = hi; hi = t; }
        if (hi == lo) return lo;
        return lo + (rand() % (hi - lo + 1));
    }
}

/**
 * @brief Loads pickup display settings (duration, color, position, etc) from ui.json.
 *
 * This is called once at game startup. It reads the "pickupDisplay" section
 * of the UI config file and applies settings like how long each message shows,
 * what color it is, where to draw it (bottom right of screen), and how many can show at once.
 *
 * @return true if the JSON was successfully loaded and parsed, false on failure.
 *
 * @note Called by:
 *   - The game initialization system at startup, once, before gameplay begins.
 */
bool DropSystem::InitDropSystemSettings()
{
    std::ifstream ifs{ "Assets/Data/ui.json", std::ios_base::binary };
    if (!ifs.is_open()) {
        std::cout << "FAILED TO OPEN UI JSON\n";
        return false;
    }
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;

    if (!Json::parseFromStream(builder, ifs, &root, &errs))
    {
        std::cout << "ui-pickups data: parse failed: " << errs << "\n";
        return false;
    }
    if (!root.isObject() || !root.isMember("pickupDisplay")) {
        std::cout << "Missing \"pickups\" in root\n";
        return false;
    }
    root = root["pickupDisplay"];

    timeTillPop = root.get("duration", 0.f).asFloat();
    fontSize = root.get("fontSize", 0.f).asFloat();
    if (root.findArray("col") && root["col"].size() == 4) {
        displayTextCol = Color{
            root["col"][0].asFloat(),root["col"][1].asFloat(),
            root["col"][2].asFloat(),root["col"][3].asFloat()
        };
    }
    if (root.findArray("pos") && root["pos"].size() == 2) {
        normDisplayPos.x = root["pos"][0].asFloat();
        normDisplayPos.y = root["pos"][1].asFloat();
    }
    textAnchor = ParseTextAlignment(root.get("textAlignment", "").asString());
    maxDisplayed = root.get("max", 0).asUInt();

    timer = timeTillPop;
    ifs.close();
    return true;
}

/**
 * @brief Spawns all the items an enemy can drop at a given position when they die.
 *
 * This is the full loot-drop pipeline:
 * 1. Looks up the enemy's DropTable in GameDB using their dropTableId.
 * 2. For each entry in the table, rolls a random number.
 * 3. If the roll succeeds (e.g. 0.35 rolls under a 0.40 chance), it builds a PickupPayload.
 * 4. The payload is handed to PickupGO::Spawn() which places the physical item on the ground.
 *
 * Multiple items can drop at once if multiple entries succeed their rolls.
 * Some entries are of type "None" this is intentional! It's a way to make
 * certain drops rarer by occupying a slot that always fails silently.
 *
 * @param dropTableId  The ID for this enemy's loot table (the same ID stored in EnemyDef).
 *                     Passed by VALUE (int).
 * @param worldPos     The position in the game world to spawn the drops at (usually the enemy's death position).
 *                     Passed by CONST REFERENCE.
 *
 * @note Called by:
 *   - Enemy::OnDeath() when an enemy dies. The enemy passes its own world position and dropTableId.
 */
void DropSystem::SpawnDrops(int dropTableId, const AEVec2& worldPos)
{
    // Retrieve the specific set of potential drops from the DB
    const DropTable* table = GameDB::GetDropTable(dropTableId);
    if (!table) return;

    // Check every single entry inside the loot table dynamically. 
    // (This means multiple items can drop at once if multiple entries succeed their chance roll)
    for (int i = 0; i < table->entryCount; ++i)
    {
        const DropEntry& e = table->entries[i];
        
        // Skip right away if it has a 0% chance
        if (e.chance <= 0.0f) continue;
        
        // Rand01() generates a decimal between 0.0 and 1.0. 
        // Example: If chance is 0.40 (40%), and Rand01() rolls 0.35 the item will spawn.
        // If it rolls 0.50 it fails and continues to the next item loop.
        if (Rand01() > e.chance) continue;

        PickupPayload payload;
        payload.type = e.type;

        if (e.type == DropType::None) {
			// Unlucky hit so we signal no drop by skipping the spawn
            continue; 
        }
        else if (e.type == DropType::Equipment)
        {
            // Equipment always drops individually, so amount is clamped to 1.
            payload.amount = 1;
            
            // If the category is None (e.g. "Any" mapped in JSON), pick a completely random item from the entire database pool
            if (e.equipmentCategory == EquipmentCategory::None)
            {
                payload.equipment = GameDB::GetRandomEquipment();
            }
            else
            {
                // Otherwise, get the exact specific item defined by the ID and category
                /* Example JSON for specific item (if we want to add them for bosses):
                {
                    "type": "Equipment",
                    "category": "Melee",     // <--- The EXACT category
                    "itemId": 1,             // <--- The EXACT item ID in that category
                    "chance": 0.4 
                }
                */
                payload.equipment = GameDB::GetEquipmentData(e.equipmentCategory, e.itemId);
            }
            if (!payload.equipment) {
                std::cout << "DEBUG DROP: Equipment drop rolled successfully, but failed to find data! Category: " << (int)e.equipmentCategory << " ID: " << e.itemId << "\n";
                continue;
            }
            std::cout << "DEBUG DROP: Successfully spawned equipment: " << payload.equipment->name << "\n";
        }
        else if (e.type == DropType::Heal) 
        {
            float playerMaxHp = GameDB::GetPlayerBaseStats().maxHP;
            payload.amount = static_cast<int>(playerMaxHp * 0.10f); // 10% max hp
            payload.equipment = nullptr;
            if (payload.amount <= 0) payload.amount = 1; // minimum 1
        }
        else
        {
            // For general consumables (Ammo, Coin), we pick a random volume inside the range setup in JSON
            payload.amount = RandRangeInt(e.minAmount, e.maxAmount);
            payload.equipment = nullptr;
            if (payload.amount <= 0) continue;
        }

        // Spawn pickup GO (uses manager registration via GameObject::Init)
        PickupGO::Spawn(worldPos, payload);
    }
}

/**
 * @brief Adds a pickup notification message to the scrolling screen log.
 *
 * When the player picks up an item, a brief message appears on the HUD showing what they got
 * (e.g. "+5 Coins", "+10 Ammo", "+Iron Sword"). This function formats that message
 * from the pickup payload and pushes it into a queue.
 * If the queue is already full (max entries reached), the oldest message is dropped to make room.
 *
 * @param payload  What was just picked up. Passed by CONST REFERENCE. Only read
 *                 the type and amount to format the display string, we never change the payload.
 *
 * @note Called by:
 *   - Player::TryPickup() - right after a successful pickup, to notify the HUD.
 */
void DropSystem::AddToPickupDisplay(const PickupPayload& payload)
{
    std::string str{"+" + std::to_string(payload.amount) + " "};
    switch (payload.type)
    {
    case DropType::Coin:
        str += (payload.amount > 1 ? "Coins" : "Coin");
        break;
    case DropType::Ammo:
        str += "Ammo";
        break;
    case DropType::Heal:
        str += "HP";
        break;
    case DropType::Buff:
		str = "+Buff"; // Placeholder for specific buff display (no buffs implemented yet for drops)
        break;
    case DropType::Equipment:
        str += payload.equipment->name;
        break;
    case DropType::None:
    default:
        return;
    }
    //If q is full, pop oldest
    if (pickupLogQueue.size() >= maxDisplayed) {
        pickupLogQueue.pop_front();
    }
    pickupLogQueue.push_back(str);
}

/**
 * @brief Draws the scrolling pickup log messages on the screen every frame.
 *
 * This renders the list of recent pickup messages ("+ 5 Coins", "+ Iron Sword") in the corner
 * of the screen. Each message stays on screen for `timeTillPop` seconds before
 * the oldest one is removed from the front of the queue.
 *
 * @param dt  Time since the last frame in seconds. Passed by VALUE.
 *
 * @note Called by:
 *   - Player::DrawUI() or the main game draw each frame while the log has messages to show.
 */
void DropSystem::PrintPickupDisplay()
{
    if (pickupLogQueue.empty()) return;

    static RenderingManager* rm{ RenderingManager::GetInstance() };
    size_t i{};
    for (std::string const& str : pickupLogQueue) {
        DrawAEText(rm->GetFont(), str.c_str(),
            NormToWorld({ normDisplayPos.x, normDisplayPos.y + (rm->GetFontHeight() * fontSize)*i}), fontSize, displayTextCol, textAnchor);
        ++i;
    }
}

void DropSystem::UpdatePickupDisplay(float dt)
{
    if (pickupLogQueue.empty()) return;

    timer -= dt;
    if (timer <= 0.f) {
        pickupLogQueue.pop_front();
        timer = timeTillPop;
    }
}

/**
 * @brief Disables and removes all active pickup GameObjects from the world.
 *
 * Called whenever the player transitions to a new map so ground loot from
 * the previous room does not bleed into the next one.
 *
 * @note Called by:
 *   - GameState::Update() at every connector transition (CSV?proc and proc?proc).
 */
void DropSystem::ClearAllPickups()
{
    auto& gos = GameObjectManager::GetInstance()->GetGameObjects();
    for (GameObject* go : gos) {
        if (!go || !go->IsEnabled()) continue;
        if (go->GetGOType() == GO_TYPE::ITEM)
            go->SetEnabled(false);
    }
    pickupLogQueue.clear();         
    timer = timeTillPop;            
}