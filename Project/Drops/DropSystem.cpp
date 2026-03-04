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

void DropSystem::SpawnDrops(int dropTableId, const AEVec2& worldPos)
{
    /* GameDB not done yet
    const DropTable* table = GameDB::GetDropTable(dropTableId);
    if (!table) return;

    for (int i = 0; i < table->entryCount; ++i)
    {
        const DropEntry& e = table->entries[i];
        if (e.chance <= 0.0f) continue;
        if (Rand01() > e.chance) continue;

        PickupPayload payload;
        payload.type = e.type;

        if (e.type == DropType::Equipment)
        {
            payload.amount = 1;
            payload.equipment = GameDB::GetEquipmentData(e.equipmentCategory, e.itemId);
            if (!payload.equipment) continue;
        }
        else
        {
            payload.amount = RandRangeInt(e.minAmount, e.maxAmount);
            payload.equipment = 0;
            if (payload.amount <= 0) continue;
        }

        // Spawn pickup GO (uses manager registration via GameObject::Init)
        PickupGO::Spawn(worldPos, payload);
    }
    */
    
	//Temporary placeholder drop logic
    // TEMP DEMO: always drop 5 coins
    PickupPayload payload{};
    payload.type = DropType::Coin;
    payload.amount = 5;

    PickupGO::Spawn(worldPos, payload);
}

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
    case DropType::Equipment:
        str += payload.equipment->name;
        break;
    default:
        return;
    }
    //If q is full, pop oldest
    if (pickupLogQueue.size() >= maxDisplayed) {
        pickupLogQueue.pop_front();
    }
    pickupLogQueue.push_back(str);
}

void DropSystem::PrintPickupDisplay(float dt)
{
    if (pickupLogQueue.empty()) return;

    timer -= dt;
    if (timer <= 0.f) {
        pickupLogQueue.pop_front();
        timer = timeTillPop;
    }
    static RenderingManager* rm{ RenderingManager::GetInstance() };
    size_t i{};
    for (std::string const& str : pickupLogQueue) {
        DrawAEText(rm->GetFont(), str.c_str(),
            NormToWorld({ normDisplayPos.x, normDisplayPos.y + (rm->GetFontHeight() * fontSize)*i}), fontSize, displayTextCol, textAnchor);
        ++i;
    }
}
