#include "PetManager.h"
#include "PetSkills.h"
#include "../RenderingManager.h"
#include <iostream>
#include <fstream>
#include <json/json.h>
#include "../File/CSV.h"

/* Flow
1. App executes, loading pet manager and game state -> LinkPlayer
2. In main menu, pet is selected, calling SetPet (can be several times)
3. Game starts -> GameState.Init -> Calls InitPetForGame to apply pet's passive
4. Game ends -> GameState.ExitState -> Clears status effects, so no need to handle that here.
*/

void PetManager::Init() {
	po = PostOffice::GetInstance();
	po->Register("PetManager", this);

	LoadPetData();

	//Init pet GO
	if (!equippedPet) {
		equippedPet = new Pet{};
	}
	//Values are TEMP
	equippedPet->Init({}, { 25,25 }, 0, MESH_SQUARE, Collision::COL_CIRCLE, { 25,25 }, CreateBitmask(1, Collision::LAYER::ENEMIES), Collision::LAYER::PET);
	equippedPet->SetEnabled(false);
	//TEMP
	SetPet(Pets::PET_1, Pets::MYTHICAL);
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
	equippedPet->SetEnabled(true);
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
void PetManager::SetPet(Pets::PET_TYPE pet, Pets::PET_RANK rank)
{
	if (pet == Pets::PET_TYPE::NONE) {
		equippedPet->isSet = false;
		return;
	}
	//Set skill ptr, other values
	Pets::PetData const& data{ petData.find(pet)->second };
	equippedPet->SetData(data, rank);
	equippedPet->GetRenderData().ReplaceTexture(data.texture.c_str(), 0);
	equippedPet->isSet = true;
}

//Deprecated. Each pet has their own rarity, need to check individual pet
Pets::PET_RANK PetManager::GetPetRank(Pets::PET_TYPE pet)
{
	switch (pet)
	{
	case Pets::PET_1:
		return Pets::MYTHICAL;
	default:
		return Pets::COMMON;
	}
}

bool PetManager::AddNewPet(Pets::PetSaveData const& newPet)
{
	if (ownedPets.size() >= MAX_PETS) return false;
	if (CSV::Write(InvFilePath, { std::to_string(newPet.id), std::to_string(newPet.rank) })) {
		//Write success
		ownedPets.push_back(newPet);
		return true;
	}
	return false;
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
		if (equippedPet && equippedPet->IsEnabled()) {
			equippedPet->CastSkill({player, equippedPet});
		}
		break;
	case PetSkillMsg::SKILL_READY:
		break;
	default:
		break;
	}
	delete message;
	return true;
}

void PetManager::LoadPetData()
{
	std::ifstream ifs{ "Assets/Data/pets.json", std::ios_base::binary };

	if (!ifs.is_open()) {
		std::cout << "PET DATA FAILED TO OPEN\n";
		return;
	}

	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;

	if (!Json::parseFromStream(builder, ifs, &root, &errs))
	{
		std::cout << "Pet data: parse failed: " << errs << "\n";
		return;
	}

	if (!root.isObject() || !root.isMember("pets") || !root["pets"].isArray()) {
		std::cout << "Pet data: missing/invalid 'pets' array\n";
		return;
	}

	//Read array of PetData objects. 
	for (Json::Value const& v : root["pets"]) {
		Pets::PetData pd{};

		pd.id = static_cast<Pets::PET_TYPE>(v.get("id", Pets::PET_TYPE::NONE).asInt());
		pd.name = v.get("name", "Pet").asString();
		//Load mods for passive
		Json::Value const* passive{ v.findArray("passive") };
		if (passive) {
			for (Json::Value const& m : v["passive"]) {
				//Each value should be a mod
				pd.passive.AddMod(StatEffects::Mod::ParseFromJSON(m));
			}
		}
		//Load multipliers
		if (v.findArray("multipliers")) {
			for (Json::Value const& m : v["multipliers"]) {
				//Each value should be a mod
				pd.multipliers.push_back(StatEffects::Mod::ParseFromJSON(m));
			}
		}
		//Rarity scalings (array of numbers)
		if (v.findArray("rarityScaling")) {
			Json::Value const& scales{ v["rarityScaling"] };
			for (int i{}; i < 6; ++i) {
				pd.rarityScaling[i] = scales[i].asInt();
			}
		}
		pd.skillCooldown = v.get("skillCooldown", 0).asFloat();
		pd.skillDesc = v.get("skillDesc", "").asString();
		pd.texture = v.get("texture", "").asString();
		pd.passive.SetIcon(pd.texture); //Set passive icon to same tex as pet
		if (v.findArray("skillElements")) {
			for (Json::Value const& m : v["skillElements"]) {
				pd.skillElements.push_back(static_cast<Elements::ELEMENT_TYPE>(m.asInt()));
			}
		}

		//Get pet skill ptr
		pd.PetSkill = PetSkills::skills[pd.id];

		petData[pd.id] = pd;

		// Pet inventory
		CSV const& inv{ CSV{InvFilePath} };
		//Each row is a pet entry
		for (unsigned r{}; r < min(inv.GetRows(), MAX_PETS); ++r) {
			Pets::PetSaveData d{};
			//Each col is the variable
			for (unsigned c{}; c < inv.GetCols(); ++c) {
				if (0 == c) {
					d.id = static_cast<Pets::PET_TYPE>(stoi(inv.GetData(r, c)));
				}
				else if (1 == c) {
					d.rank = static_cast<Pets::PET_RANK>(stoi(inv.GetData(r, c)));
				}
			}
			ownedPets.push_back(d);
		}
	}
}
