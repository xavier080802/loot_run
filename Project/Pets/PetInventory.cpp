#include "PetInventory.h"
#include <json/json.h>
#include <fstream>
#include <iostream>

namespace Pets {

    void SaveInventory(std::map<int, std::map<int, int>> const& inventory) {
        Json::Value root;
        Json::Value invArray(Json::arrayValue);

        for (auto const& outer_pair : inventory) {
            int pId = outer_pair.first; // This is the Pet ID
            for (auto const& inner_pair : outer_pair.second) {
                int pRank = inner_pair.first; // This is the Rank
                int pCount = inner_pair.second; // This is the Count

                if (pCount > 0) {
                    Json::Value entry;
                    entry["petId"] = pId;
                    entry["rank"] = pRank;
                    entry["count"] = pCount;
                    invArray.append(entry);
                }
            }
        }

        root["inventory"] = invArray;

        std::ofstream file("Assets/Data/pet_inventory.json");
        if (file.is_open()) {
            Json::StyledWriter writer;
            file << writer.write(root);
            file.close();
        }
    }

    void LoadInventory(std::map<int, std::map<int, int>>& outMap) {
        outMap.clear();
        std::ifstream ifs{ "Assets/Data/pet_inventory.json", std::ios_base::binary };

        if (!ifs.is_open()) return;

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errs;

        if (Json::parseFromStream(builder, ifs, &root, &errs)) {
            if (root.isMember("inventory") && root["inventory"].isArray()) {
                const Json::Value inv = root["inventory"];
                for (unsigned int i = 0; i < inv.size(); ++i) {
                    int id = inv[i].get("petId", 0).asInt();
                    int rank = inv[i].get("rank", 0).asInt();
                    int count = inv[i].get("count", 0).asInt();

                    outMap[id][rank] = count;
                }
            }
        }
        ifs.close();
    }

} 