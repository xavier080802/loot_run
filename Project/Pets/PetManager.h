#ifndef _PET_MANAGER_H_
#define _PET_MANAGER_H_
#include "../DesignPatterns/Singleton.h"
#include "../DesignPatterns/PostOffice.h"
#include "PetConsts.h"
#include "Pet.h"
#include "PetInventory.h" 
#include "../Actor/Player.h"
class RenderingManager;
class UIElement;
class PetManager : public Singleton<PetManager>, public PostBox
{
	friend Singleton<PetManager>;
public:
	const unsigned MAX_PETS{ 1000 };
	void Init();
	void InitPetForGame();
	void PlacePet(AEVec2 const& pos);
	void LinkPlayer(Player* playerGO);
	Player const& GetPlayer() { return *player; }
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
		if (!equippedPet || !equippedPet->isSet) return Pets::PET_TYPE::NONE;
		return equippedPet->GetPetData().id;
	}

	// Returns the rank the pet was equipped at
	Pets::PET_RANK GetEquippedRank() const
	{
		return equippedRank;
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
	Pets::PET_RANK equippedRank{ Pets::COMMON };
	UIElement* skillUI{};
	bool showTooltip{ false };
	void ShowPetTooltip();
	void LoadUIJSON();
	void LoadInventoryCounts(std::map<int, std::map<int, int>>& outMap);
};
#endif // !_PET_MANAGER_H_