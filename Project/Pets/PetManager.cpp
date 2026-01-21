#include "PetManager.h"
#include "PetSkills.h"

void PetManager::Init() {
	po = PostOffice::GetInstance();
	po->Register("PetManager", this);

	//Load pet data
	//In the future, the values should be from a file.
	Pet::PetData data1{"Basic Pet", 2, PetSkills::Pet1Skills};
	petData.insert(std::pair<Pets::PET_TYPE, Pet::PetData>(Pets::PET_1, data1));

	//Init pet GO
	equippedPet = new Pet{};
	//Values are TEMP
	equippedPet->Init({}, { 25,25 }, 0, MESH_SQUARE, COL_CIRCLE, { 25,25 }, 0, GameObject::COLLISION_LAYER::PET);

	//TEMP
	SetPet(Pets::PET_1, 0);
}

//Call when selecting pet in main menu
void PetManager::SetPet(Pets::PET_TYPE pet, int dupes)
{
	//Set skill ptr, other values
	equippedPet->SetData(petData.find(pet)->second);
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
		equippedPet->CastSkill({});
		break;
	case PetSkillMsg::SKILL_READY:
		break;
	default:
		break;
	}
	delete message;
	return true;
}
