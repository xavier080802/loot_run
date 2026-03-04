#ifndef PET_INVENTORY_H
#define PET_INVENTORY_H

#include <json/json.h>
#include <fstream>
#include <string>
#include <map>
#include "Pets/PetConsts.h"

static const char* InventoryCountPath = "Assets/Data/pet_inventory.json";

static bool LoadInventoryCounts(std::map<int, std::map<int, int>>& out) {
    std::ifstream ifs{ InventoryCountPath, std::ios::binary };
    out.clear();
    if (!ifs.is_open()) return true;
    Json::CharReaderBuilder rb;
    Json::Value root; std::string errs;
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

static bool SaveInventoryCounts(const std::map<int, std::map<int, int>>& data) {
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
    std::ofstream ofs{ InventoryCountPath, std::ios::binary | std::ios::trunc };
    if (!ofs.is_open()) return false;
    Json::StreamWriterBuilder w; w["indentation"] = "  ";
    std::unique_ptr<Json::StreamWriter> sw(w.newStreamWriter());
    sw->write(root, &ofs);
    return true;
}

static bool IncrementCount(Pets::PET_TYPE idE, Pets::PET_RANK rankE, int delta = 1) {
    std::map<int, std::map<int, int>> data;
    LoadInventoryCounts(data);
    int id = static_cast<int>(idE), rank = static_cast<int>(rankE);
    data[id][rank] += delta;
    if (data[id][rank] <= 0) data[id].erase(rank);
    if (data[id].empty()) data.erase(id);
    return SaveInventoryCounts(data);
}

#endif#pragma once
