#ifndef _SHOP_FUNCTIONS_H
#define _SHOP_FUNCTIONS_H

#include "AEEngine.h"
#include "../../Actor/StatsTypes.h"
#include <string>
#include <map>

class ShopFunctions
{
public:
	// Singleton access to ensure stats persist across states
	static ShopFunctions* GetInstance();

	// Load/Save from player_data.json
	void loadPlayerShopUpgrades();
	void savePlayerShopUpgrades();

	//buy upgrade by purchase amount
	bool buyShopUpgrade(STAT_TYPE stat);

	//refund upgrade by purchase amount
	void sellShopUpgrade(STAT_TYPE stat);

	//refund all (refund all upgrades for each stat)
	void sellAllShopUpgrades();

	// Getters for UI/Calculation
	int getUpgradeLevel(STAT_TYPE stat);
	float getStatBonus(STAT_TYPE stat);
	int calculatePrice(STAT_TYPE stat);

	// Purchase amount toggles (x1, x10, etc.)
	void setPurchaseMultiplier(int count) { currentMultiplier = count; }
	int getPurchaseMultiplier() const { return currentMultiplier; }

	int getMoney() const { return money; }

	// Adds coins to the persistent wallet and saves the file.
	void addMoney(int amount);

	// Endless mode high score (seconds survived)
	float getEndlessHighScore() const { return endlessHighScore; }
	// Only updates and saves if 'seconds' is a new best.
	void updateEndlessHighScore(float seconds);

private:
	ShopFunctions(); // Private constructor for Singleton

	struct ShopStat {
		float statIncrement = 0.1f;
		int basePrice = 100;
		int priceScaling = 100;
	};

	// Maps STAT_TYPE to the current level of that upgrade
	std::map<STAT_TYPE, int> upgradeLevels;

	// Internal helper to define default shop constants
	ShopStat getStatSettings(STAT_TYPE stat);

	int money = 0;
	float endlessHighScore = 0.f;
	int currentMultiplier = 1; // Default x1

	std::string getStatName(STAT_TYPE stat)
	{
		switch (stat)
		{
		case STAT_TYPE::MAX_HP:   return "HEALTH";
		case STAT_TYPE::DEF:      return "DEFENSE";
		case STAT_TYPE::ATT:      return "ATTACK";
		case STAT_TYPE::ATT_SPD:  return "ATTACK SPEED";
		case STAT_TYPE::MOVE_SPD: return "MOVE SPEED";
		default:                  return "UNKNOWN";
		}
	}

	static constexpr const char* PLAYER_DATA_PATH = "Assets/Data/Player/player_data.json";
};

#endif // !_SHOP_FUNCTIONS_H
