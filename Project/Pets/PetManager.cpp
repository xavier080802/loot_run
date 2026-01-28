#include "PetManager.h"
#include "PetSkills.h"
#include "../RenderingManager.h"
#include <iostream>

/* Flow
1. App executes, loading pet manager and game state -> LinkPlayer
2. In main menu, pet is selected, calling SetPet (can be several times)
3. Game starts -> GameState.Init -> Calls InitPetForGame to apply pet's passive
4. Game ends -> GameState.ExitState -> Clears status effects, so no need to handle that here.
*/

void PetManager::Init() {
	po = PostOffice::GetInstance();
	po->Register("PetManager", this);

	StatEffects::StatusEffect passive{ nullptr, -1, 1, "Pet Passive" };

	//Load pet data
	//In the future, the values should be from a file.
	Pet::PetData data1{ "Basic Pet", 2, PetSkills::Pet1Skill, "Assets/finn.png", passive};
	data1.passive.AddMod(StatEffects::Mod(10, StatEffects::MULTIPLICATIVE, STAT_TYPE::ATT));
	petData.insert(std::pair<Pets::PET_TYPE, Pet::PetData>(Pets::PET_1, data1));

	//Init pet GO
	equippedPet = new Pet{};
	//Values are TEMP
	equippedPet->Init({}, { 25,25 }, 0, MESH_SQUARE, COL_CIRCLE, { 25,25 }, CreateBitmask(1, GameObject::COLLISION_LAYER::ENEMIES), GameObject::COLLISION_LAYER::PET);
	equippedPet->SetEnabled(false);
	//TEMP
	//SetPet(Pets::PET_1, 0);
}

void PetManager::InitPetForGame()
{
	if (!equippedPet->isSet) return;
	if (!player) {
		std::cout << "Player not linked to PetManager.\n";
		return;
	}
	//Apply passive
	player->ApplyStatusEffect(new StatEffects::StatusEffect{ equippedPet->GetPetData().passive }, player);
	//Reset pet vars
	equippedPet->Reset();
}

void PetManager::LinkPlayer(Player* playerGO)
{
	player = playerGO;
}

void PetManager::PlacePet(AEVec2 const& pos)
{
	if (!equippedPet) return;
	equippedPet->SetPos(pos);
}

//Call when selecting pet in main menu
void PetManager::SetPet(Pets::PET_TYPE pet, int dupes)
{
	if (pet == Pets::PET_TYPE::NONE) {
		equippedPet->isSet = false;
		return;
	}
	//Set skill ptr, other values
	Pet::PetData const& data{ petData.find(pet)->second };
	equippedPet->SetData(data);
	equippedPet->GetRenderData().ReplaceTexture(data.texture.c_str(), 0);
	equippedPet->isSet = true;
	equippedPet->SetEnabled(true);
}

Pets::PET_RANK PetManager::GetPetRank(Pets::PET_TYPE pet)
{
	switch (pet)
	{
	case Pets::PET_1:
		return Pets::SSR;
	default:
		return Pets::SR;
	}
}

bool PetManager::Handle(Message* message)
{
	PetSkillMsg* msg = dynamic_cast<PetSkillMsg*>(message);
	if (!msg) {
		delete message;
		return false;
	}
	//Read message
	switch (msg->type)
	{
	case PetSkillMsg::CAST_SKILL:
		equippedPet->CastSkill({player, equippedPet});
		break;
	case PetSkillMsg::SKILL_READY:
		break;
	default:
		break;
	}
	delete message;
	return true;
}
