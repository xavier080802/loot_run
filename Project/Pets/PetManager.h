#ifndef _PET_MANAGER_H_
#define _PET_MANAGER_H_
#include "../DesignPatterns/Singleton.h"
#include "../DesignPatterns/PostOffice.h"
#include "PetConsts.h"
#include "Pet.h"
#include "../Actor/Player.h"

class RenderingManager;
class UIElement;

class PetManager : public Singleton<PetManager>, public PostBox
{
	friend Singleton<PetManager>;
public:
	const unsigned MAX_PETS{ 1000 };

	void Init();
	//When game state inits, call to reset pet data
	void InitPetForGame();
	//Place pet somewhere in world
	void PlacePet(AEVec2 const& pos);
	//Pass in player GO so pet can follow, and other stuff
	void LinkPlayer(Player* playerGO);
	Player const& GetPlayer() { return *player; }

	//Sets the pet based on the type.
	void SetPet(Pets::PET_TYPE pet, Pets::PET_RANK rank);
	Pet* GetEquippedPet() { return equippedPet; }
	bool PetHasSkill() const;

	//Add a new pet to the inventory.
	//Returns success. If max limit is reached, returns false
	bool AddNewPet(Pets::PET_TYPE type, Pets::PET_RANK rank);

	bool Handle(Message* message) override;

	void DrawUI();

	// Get the internal map for the UI loop
	std::map<int, std::map<int, int>> const& GetInventory() const { return ownedPets; }
	std::map<Pets::PET_TYPE, Pets::PetData> const& GetPetDataMap() const { return petData; }

private:
	void LoadPetData();
	PostOffice* po{};
	RenderingManager* rm{};
	//Constant data. info for each pet, not player-specific
	std::map<Pets::PET_TYPE, Pets::PetData> petData{};
	Pet* equippedPet{};
	Player* player{};
	std::string extraDesc{}; //Pet passive/mults/etc

	//Pet inventory (Updated to nested map for JSON compatibility)
	std::map<int, std::map<int, int>> ownedPets{};

	//UI
	UIElement* skillUI{};
	bool showTooltip{ false };
	void ShowPetTooltip();
	void LoadUIJSON();
};
#endif // !_PET_MANAGER_H_