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
	Pets::PET_RANK GetPetRank(Pets::PET_TYPE pet);

	//Add a new pet to the inventory.
	//Returns success. If max limit is reached, returns false
	bool AddNewPet(Pets::PetSaveData const& newPet);

	bool Handle(Message* message) override;

	void DrawUI();

private:
	const std::string InvFilePath{ "Assets/Data/pet_inv.csv" };
	void LoadPetData();
	PostOffice* po{};
	RenderingManager* rm{};
	//Constant data. info for each pet, not player-specific
	std::map<Pets::PET_TYPE, Pets::PetData> petData{};
	Pet* equippedPet{};
	Player* player{};

	//Pet inventory
	std::vector<Pets::PetSaveData> ownedPets{};

	//UI
	UIElement* skillUI{};
	f32 descFontSize{ 0.35f };
	f32 timerFontSize{ 0.5f };
	f32 padding{ 0.02f };
	bool showTooltip{ false };
	void ShowPetTooltip();
};
#endif // !_PET_MANAGER_H_


