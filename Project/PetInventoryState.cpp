#include "PetInventoryState.h"
#include "Pets/PetManager.h"
#include "AEEngine.h"
#include <fstream>
#include <iostream>

// Standard path for the inventory JSON
static const char* InventoryPath = "Assets/Data/pet_inventory.json";

// --- GLOBAL DATA HELPERS ---
// These are defined here so other states (like Gacha) can link to them.

bool LoadInventoryCounts(std::map<int, std::map<int, int>>& out) {
    std::ifstream ifs{ InventoryPath, std::ios::binary };
    out.clear();
    if (!ifs.is_open()) return true; // File doesn't exist yet, return empty but success

    Json::CharReaderBuilder rb;
    Json::Value root;
    std::string errs;

    if (!Json::parseFromStream(rb, ifs, &root, &errs)) return false;
    if (!root.isMember("counts") || !root["counts"].isObject()) return false;

    for (auto const& idKey : root["counts"].getMemberNames()) {
        int id = std::stoi(idKey);
        Json::Value const& ranks = root["counts"][idKey];
        for (auto const& rankKey : ranks.getMemberNames()) {
            int rank = std::stoi(rankKey);
            int cnt = ranks[rankKey].asInt();
            if (cnt > 0) out[id][rank] = cnt;
        }
    }
    return true;
}

bool SaveInventoryCounts(const std::map<int, std::map<int, int>>& data) {
    Json::Value root;
    Json::Value counts(Json::objectValue);

    for (auto const& [id, rankmap] : data) {
        Json::Value rv(Json::objectValue);
        for (auto const& [rank, cnt] : rankmap) {
            if (cnt <= 0) continue;
            rv[std::to_string(rank)] = cnt;
        }
        if (!rv.empty()) counts[std::to_string(id)] = rv;
    }

    root["counts"] = counts;
    std::ofstream ofs{ InventoryPath, std::ios::binary | std::ios::trunc };
    if (!ofs.is_open()) return false;

    Json::StreamWriterBuilder w;
    w["indentation"] = "  ";
    std::unique_ptr<Json::StreamWriter> sw(w.newStreamWriter());
    sw->write(root, &ofs);
    return true;
}

bool IncrementCount(Pets::PET_TYPE idE, Pets::PET_RANK rankE, int delta) {
    std::map<int, std::map<int, int>> data;
    LoadInventoryCounts(data);
    int id = static_cast<int>(idE), rank = static_cast<int>(rankE);
    data[id][rank] += delta;

    // Cleanup if count hits zero
    if (data[id][rank] <= 0) data[id].erase(rank);
    if (data[id].empty()) data.erase(id);

    return SaveInventoryCounts(data);
}

bool DecrementCount(Pets::PET_TYPE idE, Pets::PET_RANK rankE, int delta) {
    return IncrementCount(idE, rankE, -delta);
}

// --- PET INVENTORY STATE METHODS ---

void PetInventoryState::LoadState() {
    // Fonts are usually loaded in a global manager, but set ID here
    mFontId = 0;
}

void PetInventoryState::InitState() {
    // Always refresh data from file when entering the inventory
    LoadInventoryCounts(mInventoryData);
}

void PetInventoryState::GetRankColor(int rankID, float& r, float& g, float& b) {
    switch (rankID) {
    case 0: // Common
        r = 1.0f; g = 1.0f; b = 1.0f; break; // White
    case 1: // Rare
        r = 0.2f; g = 0.6f; b = 1.0f; break; // Sky Blue
    case 2: // Epic
        r = 0.7f; g = 0.1f; b = 1.0f; break; // Purple
    case 3: // Legendary
        r = 1.0f; g = 0.6f; b = 0.0f; break; // Orange
    default:
        r = 0.6f; g = 0.6f; b = 0.6f; break;
    }
}

void PetInventoryState::Update(double dt) {
    // Exit logic
    if (AEInputCheckTriggered(AE_KEY_ESCAPE)) {
        GameStateManager::GetInstance()->SetNextGameState("MainMenuState");
    }

    // --- MOUSE & CLICK LOGIC ---
    s32 mx, my;
    AEInputGetCursorPosition(&mx, &my);

    // Convert to normalized screen space (-1 to 1)
    float mouseX = ((float)mx / AEGfxGetWinMaxX()) * 2.0f - 1.0f;
    float mouseY = -(((float)my / AEGfxGetWinMaxY()) * 2.0f - 1.0f);

    float xStart = -0.7f, yStart = 0.5f;
    float xSpace = 0.35f, ySpace = 0.25f;
    float hitBox = 0.12f;
    int cols = 5, index = 0;

    for (auto const& [petID, rankMap] : mInventoryData) {
        for (auto const& [rankID, count] : rankMap) {
            float px = xStart + (index % cols) * xSpace;
            float py = yStart - (index / cols) * ySpace;

            // Click detection
            if (mouseX > px - hitBox && mouseX < px + hitBox &&
                mouseY > py - hitBox && mouseY < py + hitBox) {

                if (AEInputCheckTriggered(AE_MOUSE_LEFT)) {
                    EquipFromInventory(petID, rankID);
                }
            }
            index++;
        }
    }
}

void PetInventoryState::Draw() {
    // Background Header
    AEGfxPrint(mFontId, "--- PET INVENTORY ---", -0.25f, 0.85f, 1.4f, 1, 1, 1, 1);
    AEGfxPrint(mFontId, "[ESC] TO RETURN", -0.95f, -0.9f, 0.8f, 1, 1, 1, 1);

    auto const& dataMap = PetManager::GetInstance()->GetPetDataMap();
    int index = 0;

    for (auto const& [petID, rankMap] : mInventoryData) {
        for (auto const& [rankID, count] : rankMap) {
            float px = -0.7f + (index % 5) * 0.35f;
            float py = 0.5f - (index / 5) * 0.25f;

            auto it = dataMap.find(static_cast<Pets::PET_TYPE>(petID));
            if (it != dataMap.end()) {
                float r, g, b;
                GetRankColor(rankID, r, g, b);

                // 1. Draw Pet Name (Color-coded by Rarity)
                AEGfxPrint(mFontId, it->second.name.c_str(), px - 0.08f, py + 0.05f, 1.0f, r, g, b, 1.0f);

                // 2. Draw Stack Count (Yellow)
                std::string stackStr = "x" + std::to_string(count);
                AEGfxPrint(mFontId, stackStr.c_str(), px + 0.06f, py - 0.04f, 0.8f, 1, 1, 0, 1);

                // 3. Draw Equipped Label (Green)
                Pet* active = PetManager::GetInstance()->GetEquippedPet();
                if (active && active->isSet &&
                    (int)active->GetPetData().id == petID &&
                    (int)active->GetRank() == rankID) {
                    AEGfxPrint(mFontId, "[ACTIVE]", px - 0.08f, py - 0.12f, 0.7f, 0, 1, 0, 1);
                }
            }
            index++;
        }
    }

    if (mInventoryData.empty()) {
        AEGfxPrint(mFontId, "No pets found in inventory.", -0.25f, 0.0f, 1.0f, 1, 1, 1, 1);
    }
}

void PetInventoryState::EquipFromInventory(int petID, int rankID) {
    PetManager::GetInstance()->SetPet(static_cast<Pets::PET_TYPE>(petID), static_cast<Pets::PET_RANK>(rankID));
    // Provide some console feedback
    std::cout << "Switching to Pet ID: " << petID << " (Rank: " << rankID << ")" << std::endl;
}

void PetInventoryState::ExitState() {
    mInventoryData.clear();
}

void PetInventoryState::UnloadState() {
    // Clean up textures if you loaded them in LoadState
}