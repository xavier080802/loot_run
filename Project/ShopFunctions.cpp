#include "ShopFunctions.h"
#include <iostream>
#include <fstream>
#include <json/json.h>

ShopFunctions* ShopFunctions::GetInstance()
{
    static ShopFunctions instance;
    return &instance;
}

ShopFunctions::ShopFunctions()
{
    loadPlayerShopUpgrades();
}

void ShopFunctions::loadPlayerShopUpgrades()
{
    // Initialize all stats to level 0 (load from file when file saving is done)
    upgradeLevels[STAT_TYPE::ATT] = 0;
    upgradeLevels[STAT_TYPE::ATT_SPD] = 0;
    upgradeLevels[STAT_TYPE::MOVE_SPD] = 0;
    upgradeLevels[STAT_TYPE::MAX_HP] = 0;
    upgradeLevels[STAT_TYPE::DEF] = 0;

    // Load persistent fields from player_data.json
    std::ifstream file(PLAYER_DATA_PATH);
    if (!file.is_open()) {
        std::cout << "[ShopFunctions] player_data.json not found, using defaults.\n";
        return;
    }

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, file, &root, &errs)) {
        std::cout << "[ShopFunctions] Failed to parse player_data.json: " << errs << "\n";
        return;
    }

    money              = root.get("totalCoins",       0).asInt();
    endlessHighScore   = root.get("endlessHighScore", 0.0f).asFloat();
    
    Json::Value upgradesNode = root["upgrades"];
    if (upgradesNode.isObject()) {
        upgradeLevels[STAT_TYPE::ATT]      = upgradesNode.get("ATT", 0).asInt();
        upgradeLevels[STAT_TYPE::ATT_SPD]  = upgradesNode.get("ATT_SPD", 0).asInt();
        upgradeLevels[STAT_TYPE::MOVE_SPD] = upgradesNode.get("MOVE_SPD", 0).asInt();
        upgradeLevels[STAT_TYPE::MAX_HP]   = upgradesNode.get("MAX_HP", 0).asInt();
        upgradeLevels[STAT_TYPE::DEF]      = upgradesNode.get("DEF", 0).asInt();
    }
    
    std::cout << "[ShopFunctions] Loaded: " << money << " coins, "
              << "best Endless run: " << endlessHighScore << "s\n";
}

void ShopFunctions::savePlayerShopUpgrades()
{
    // Read existing file first
    Json::Value root;
    {
        std::ifstream file(PLAYER_DATA_PATH);
        if (file.is_open()) {
            Json::CharReaderBuilder builder;
            std::string errs;
            Json::parseFromStream(builder, file, &root, &errs);
        }
    }

    root["totalCoins"]       = money;
    root["endlessHighScore"] = endlessHighScore;

    Json::Value upgradesNode(Json::objectValue);
    upgradesNode["ATT"]      = upgradeLevels[STAT_TYPE::ATT];
    upgradesNode["ATT_SPD"]  = upgradeLevels[STAT_TYPE::ATT_SPD];
    upgradesNode["MOVE_SPD"] = upgradeLevels[STAT_TYPE::MOVE_SPD];
    upgradesNode["MAX_HP"]   = upgradeLevels[STAT_TYPE::MAX_HP];
    upgradesNode["DEF"]      = upgradeLevels[STAT_TYPE::DEF];
    root["upgrades"]         = upgradesNode;

    std::ofstream out(PLAYER_DATA_PATH);
    if (!out.is_open()) {
        std::cout << "[ShopFunctions] Could not open player_data.json for writing.\n";
        return;
    }

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "    ";
    out << Json::writeString(writer, root);
    std::cout << "[ShopFunctions] Saved: " << money << " coins, "
              << "best Endless run: " << endlessHighScore << "s\n";
}

void ShopFunctions::addMoney(int amount)
{
    money += amount;
    if (money < 0) money = 0;
    savePlayerShopUpgrades();
    std::cout << "[ShopFunctions] addMoney(" << amount << ") -> total " << money << "\n";
}

void ShopFunctions::updateEndlessHighScore(float seconds)
{
    if (seconds > endlessHighScore) {
        endlessHighScore = seconds;
        savePlayerShopUpgrades();
        std::cout << "[ShopFunctions] New Endless high score: " << seconds << "s\n";
    }
}

int ShopFunctions::calculatePrice(STAT_TYPE stat)
{
    ShopStat settings = getStatSettings(stat);
    int currentLevel = upgradeLevels[stat];

    // BasePrice + (Level * Scaling)
    return settings.basePrice + (currentLevel * settings.priceScaling);
}

bool ShopFunctions::buyShopUpgrade(STAT_TYPE stat)
{
    int cost = calculatePrice(stat);

    if (money >= cost)
    {
        money -= cost;
        upgradeLevels[stat]++;
        savePlayerShopUpgrades(); // persist after every purchase
        std::cout << "Purchased upgrade for stat " << getStatName(stat) << ". New Level: " << upgradeLevels[stat] << std::endl;
        return true;
    }

    std::cout << "Not enough money! Need: " << cost << std::endl << "You currently have: " << money << std::endl;
    return false;
}

void ShopFunctions::sellShopUpgrade(STAT_TYPE stat)
{
    if (upgradeLevels[stat] > 0)
    {
        upgradeLevels[stat]--;
        int refund = calculatePrice(stat);
        money += refund;
        savePlayerShopUpgrades(); // persist after every refund
    }
}

void ShopFunctions::sellAllShopUpgrades()
{
    for (auto& pair : upgradeLevels)
    {
        while (pair.second > 0)
        {
            sellShopUpgrade(pair.first);
        }
    }
}

int ShopFunctions::getUpgradeLevel(STAT_TYPE stat)
{
    return upgradeLevels[stat];
}

float ShopFunctions::getStatBonus(STAT_TYPE stat)
{
    return upgradeLevels[stat] * 0.1f;
}

ShopFunctions::ShopStat ShopFunctions::getStatSettings(STAT_TYPE stat)
{
    ShopStat s;
    s.basePrice = 100;
    s.priceScaling = 100;
    s.statIncrement = 0.1f;
    return s;
}