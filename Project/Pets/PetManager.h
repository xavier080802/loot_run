#ifndef PETMANAGER_H_
#define PETMANAGER_H_

#include "../DesignPatterns/Singleton.h"
#include "../DesignPatterns/PostOffice.h"
#include "PetConsts.h"
#include "Pet.h"
#include "PetInventory.h" 
#include "../Actor/Player.h"
class RenderingManager;
class UIElement;
class TileMap;

class PetManager : public Singleton<PetManager>, public PostBox
{
	friend Singleton<PetManager>;
public:
	const unsigned MAX_PETS{ 1000 };
	void Init();
	//When game state inits, call to reset pet data
	void InitPetForGame(TileMap const& tilemap);
	//Call when changing tilemap
	void SetTilemap(TileMap const& tilemap);
	//Place pet somewhere in world
	void PlacePet(AEVec2 const& pos);
	void LinkPlayer(Player* playerGO);
	Player const& GetPlayer() const { return *player; }
	Player& GetPlayer() { return *player; }

	// Set status as "Pet not set" without de-equipping the pet
	void UnsetPet();

	//Sets the pet based on the type.
	void SetPet(Pets::PET_TYPE pet, Pets::PET_RANK rank);
	Pet* GetEquippedPet() { return equippedPet; }
	bool PetHasSkill() const;
	bool AddNewPet(Pets::PetSaveData const& newPet);
	bool Handle(Message* message) override;
	void DrawUI();
	void SaveInventoryToJSON();
	std::map<int, std::map<int, int>> const& GetInventory() const { return ownedPets; }
	std::map<Pets::PET_TYPE, Pets::PetData> const& GetPetDataMap() const { return petData; }

	// Returns the PET_TYPE of the currently equipped pet, or NONE if nothing is set
	Pets::PET_TYPE GetEquippedType() const
	{
		if (!equippedPet) return Pets::PET_TYPE::NONE;
		return equippedPet->GetPetData().id;
	}

	// Returns the rank the pet was equipped at
	Pets::PET_RANK GetEquippedRank() const
	{
		return equippedPet ? equippedPet->GetRank() : Pets::PET_RANK::COMMON;
	}

private:
	void LoadPetData();
	void CreatePet();
	PostOffice* po{};
	RenderingManager* rm{};
	std::map<Pets::PET_TYPE, Pets::PetData> petData{};
	Pet* equippedPet{};
	std::pair<Pets::PET_TYPE, Pets::PET_RANK> selectedPetInfo{};
	Player* player{};
	std::string extraDesc{};
	std::map<int, std::map<int, int>> ownedPets{};
	// Stores the rank passed to SetPet so PetState can restore the highlight on re-entry
	UIElement* skillUI{};
	bool showTooltip{ false };
	void ShowPetTooltip();
	void LoadUIJSON();
	void LoadInventoryCounts(std::map<int, std::map<int, int>>& outMap);
};

#endif // PETMANAGER_H_

