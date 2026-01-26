#ifndef _PET_MANAGER_H_
#define _PET_MANAGER_H_
#include "../DesignPatterns/Singleton.h"
#include "../DesignPatterns/PostOffice.h"
#include "PetConsts.h"
#include "Pet.h"
#include "../Actor/Player.h"

class PetManager : public Singleton<PetManager>, public PostBox
{
	friend Singleton<PetManager>;
public:
	void Init();
	//Place pet somewhere in world
	void PlacePet(AEVec2 const& pos);
	//Pass in player GO so pet can follow, and other stuff
	void LinkPlayer(Player* playerGO);
	Player const& GetPlayer() { return *player; }

	//Sets the pet based on the type.
	void SetPet(Pets::PET_TYPE pet, int dupes);

	Pets::PET_RANK GetPetRank(Pets::PET_TYPE pet);

	bool Handle(Message* message) override;

private:
	PostOffice* po{};
	//Constant data. info for each pet, not player-specific
	std::map<Pets::PET_TYPE, Pet::PetData> petData{};
	Pet* equippedPet{};
	Player* player{};

	//TODO: pet inventory, with the dupe levels and stuff.

	//TODO: databank, load from file or smthing. All the numerical values.
};
#endif // !_PET_MANAGER_H_


