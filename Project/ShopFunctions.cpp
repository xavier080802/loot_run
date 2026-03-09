#include "ShopFunctions.h"
#include <iostream>

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
}

void ShopFunctions::savePlayerShopUpgrades()
{
    // Save current upgradeLevels map to a file.
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