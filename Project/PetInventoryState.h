#ifndef PET_INVENTORY_STATE_H
#define PET_INVENTORY_STATE_H

#include "../GameStateManager.h"
#include "Pets/PetConsts.h"
#include <json/json.h>
#include <map>
#include <string>

// Global Data Helpers
bool LoadInventoryCounts(std::map<int, std::map<int, int>>& out);
bool SaveInventoryCounts(const std::map<int, std::map<int, int>>& data);
bool IncrementCount(Pets::PET_TYPE idE, Pets::PET_RANK rankE, int delta = 1);

class PetInventoryState : public State {
public:
    void LoadState() override;
    void InitState() override;
    void ExitState() override;
    void UnloadState() override;
    void Update(double dt) override;
    void Draw() override;

private:
    void EquipFromInventory(int petID, int rankID);
    // Returns R, G, B via pointers based on rank
    void GetRankColor(int rankID, float& r, float& g, float& b);

    std::map<int, std::map<int, int>> mInventoryData;
    s8 mFontId = 0;
};

#endif