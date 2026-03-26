#include "PetManager.h"
#include "PetInventory.h" 
#include "../RenderingManager.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <json/json.h>
#include <sstream> 
#include "../File/CSV.h"
#include "../UI/UIElement.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/MatrixUtils.h"
#include "../Helpers/CoordUtils.h"
#include "../Helpers/Vec2Utils.h"
#include "../TileMap.h"

/* Flow
1. App executes, loading pet manager and game state -> LinkPlayer
2. In main menu, pet is selected, calling SetPet (can be several times)
3. Game starts -> GameState.Init -> Calls InitPetForGame to create the Pet class and apply pet's 

4. Game ends -> GameState.ExitState -> Clears status effects, so no need to handle that here.
5. Program ends -> GO Manager deleted the created pet classes.

Implementing new pet:
 1. Create the class derived from Pet
 2. Add pet id to PET_TYPE
 3. Under GameObjectManager's FetchGO, return the new pet class when the type is called
 4. Within the new pet class, override GetPetGOType to return the new PET_TYPE.
 5. Implement DoSkill and other stuff as necessary
 6. Update the pet json
*/

namespace {
	f32 descFontSize{ 0.35f };
	f32 timerFontSize{ 0.5f };
	AEVec2 padding{ 0.02f, 0.02f };
	AEVec2 iconPos{ -1,-1 };
	f32 iconSize{};
	f32 lineSpace{};
	Color textCol{ 0,0,255,255 };
	Color tooltipBgCol{ 255,255,255,255 };
	Color timerTextCol{ 0,0,0,255 };
	Color onCooldownCol{ 155,155,155,155 };
	Color availableCol{ 100,255,100,255 };
	TextOriginPos tooltipAlignment{};
	TextboxOriginPos boxAlignment{};
	bool showTimerUnit{};
}

void PetManager::Init() {
	po = PostOffice::GetInstance();
	po->Register("PetManager", this);
	rm = RenderingManager::GetInstance();

	LoadPetData();

	//UI
	LoadUIJSON();
	skillUI = new UIElement{ NormToWorld(iconPos) + iconSize * 0.5f, {iconSize, iconSize}, 1, Collision::COL_RECT };
	skillUI->SetHoverCallback([this](bool) {showTooltip = true; });
}

void PetManager::InitPetForGame(TileMap const& tilemap)
{
	CreatePet();

	if (!equippedPet || !equippedPet->isSet) {
		//No selected / pet invalid
		extraDesc = "";
		return;
	}

	if (!player) {
		std::cout << "Player not linked to PetManager.\n";
		return;
	}
	equippedPet->Reset();
	PlacePet(player->GetPos());
	equippedPet->Setup(*player);
	equippedPet->SetTilemap(tilemap);
	//Apply passive
	player->ApplyStatusEffect(new StatEffects::StatusEffect{ equippedPet->GetPetData().passive }, player);

	//Generate description for passive

	std::stringstream s;
	Pets::PetData const& d{ equippedPet->GetPetData() };
	//Generate description for the skills
	if (PetHasSkill()) {
		//Cooldown
		s << "\n[Cooldown " << std::fixed << std::setprecision(1) << d.skillCooldown << "s]\n";
		//Scalings
		bool flag = false; //First listing
		for (StatEffects::Mod const& m : d.multipliers) {
			s << (flag ? " + " : "") << std::fixed << std::setprecision(1) << m.value
				<< (m.mathType == StatEffects::MATH_TYPE::MULTIPLICATIVE ? "% " : " ")
				<< (m.mathType == StatEffects::MATH_TYPE::MULTIPLICATIVE ? StatTypeToString(m.stat) : "");
			flag = true;
		}
	}
	s << "\nPassive: ";
	for (StatEffects::Mod const& m : d.passive.GetMods()) {
		s << (m.value > 0 ? "+" : "") << std::fixed << std::setprecision(1) << m.value
			<< (m.mathType == StatEffects::MATH_TYPE::MULTIPLICATIVE ? "% " : " ")
			<< StatTypeToString(m.stat) << " ";
	}
	extraDesc = s.str();
}

void PetManager::SetTilemap(TileMap const& tilemap) {
	if (equippedPet) {
		equippedPet->SetTilemap(tilemap);
	}
}

void PetManager::LinkPlayer(Player* playerGO)
{
	player = playerGO;
}

void PetManager::PlacePet(AEVec2 const& pos)
{
	if (!equippedPet) return;
	equippedPet->SetPos(pos);
	equippedPet->ResetPathfinder();
}

//Call when selecting pet in main menu
void PetManager::SetPet(Pets::PET_TYPE pet, Pets::PET_RANK rank)
{
	selectedPetInfo.first = pet;
	selectedPetInfo.second = rank;
}

bool PetManager::PetHasSkill() const
{
	return equippedPet && equippedPet->isSet && equippedPet->HasSkill();
}

bool PetManager::AddNewPet(Pets::PetSaveData const& newPet)
{
	// Increments the count for the specific Pet ID and Rank
	ownedPets[static_cast<int>(newPet.id)][static_cast<int>(newPet.rank)]++;
	//SaveInventoryToJSON();

	return true;
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
			equippedPet->CastSkill({ player });
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

void PetManager::DrawUI()
{
	if (!equippedPet || !equippedPet->isSet || !PetHasSkill()) return;

	//Available - draw a green box
	if (!equippedPet->IsOnCooldown()) {
		DrawTintedMesh(GetTransformMtx(skillUI->GetPos(), 0, skillUI->GetSize()),
			rm->GetMesh(MESH_SQUARE), nullptr,
			availableCol, 220);
	}

	DrawTintedMesh(GetTransformMtx(skillUI->GetPos(), 0, skillUI->GetSize()),
		rm->GetMesh(MESH_SQUARE), rm->LoadTexture(equippedPet->GetPetData().textures.at(0)),
		equippedPet->IsOnCooldown() ? onCooldownCol : Color{ 255,255,255,255 }, 255);

	//Write cooldown
	if (equippedPet->IsOnCooldown()) {
		DrawAEText(rm->GetFont(), std::to_string((int)equippedPet->GetCDTimer()) + (showTimerUnit ? "s" : ""), skillUI->GetPos(), timerFontSize,
			0, timerTextCol, TEXT_MIDDLE);
	}
	else { //Write key above the box
		DrawAEText(rm->GetFont(), "[R]", skillUI->GetPos() + AEVec2{ 0, skillUI->GetSize().y * 0.5f + 5}, timerFontSize,
			0, availableCol, TEXT_LOWER_MIDDLE);
	}

	if (showTooltip) {
		ShowPetTooltip();
		showTooltip = false;
	}

	//equippedPet->DrawPath();
}

void PetManager::ShowPetTooltip()
{
	std::string txt{ equippedPet->GetPetData().skillDesc + extraDesc };

	s32 mx{}, my{};
	AEInputGetCursorPosition(&mx, &my);
	AEVec2 mP = ScreenToWorld(AEVec2{ (float)mx, (float)my });

	//Draw text box
	DrawAETextbox(rm->GetFont(), txt, AEVec2{ (float)mP.x, (float)mP.y },
		AEGfxGetWinMaxX() * 0.7f, descFontSize, lineSpace, textCol, tooltipAlignment, TextboxOriginPos::BOTTOM,
		TextboxBgCfg{ padding, tooltipBgCol, 255, rm->GetMesh(MESH_SQUARE) });
}

void PetManager::LoadUIJSON()
{
	std::ifstream ifs{ "Assets/Data/ui.json", std::ios_base::binary };
	if (!ifs.is_open()) return;

	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;

	if (Json::parseFromStream(builder, ifs, &root, &errs) && root.isMember("pet_ui"))
	{
		Json::Value ui = root["pet_ui"];
		descFontSize = ui.get("descFontSize", 0.35f).asFloat();
		timerFontSize = ui.get("timerFontSize", 0.5f).asFloat();
		if (ui["padding"].isArray()) {
			padding.x = ui["padding"][0].asFloat();
			padding.y = ui["padding"][1].asFloat();
		}
		if (ui["pos"].isArray()) {
			iconPos.x = ui["pos"][0].asFloat();
			iconPos.y = ui["pos"][1].asFloat();
		}
		iconSize = ui.get("size", 75).asFloat();
		lineSpace = ui.get("lineSpace", 0.05f).asFloat();
		showTimerUnit = ui.get("showTimerUnit", true).asBool();
		if (ui.isMember("textCol") && ui["textCol"].size() == 4) {
			textCol = Color{ ui["textCol"][0].asFloat(), ui["textCol"][1].asFloat(),
			ui["textCol"][2].asFloat() ,ui["textCol"][3].asFloat() };
		}
		if (ui.isMember("boxCol") && ui["boxCol"].size() == 4) {
			tooltipBgCol = Color{ ui["boxCol"][0].asFloat(), ui["boxCol"][1].asFloat(),
			ui["boxCol"][2].asFloat() ,ui["boxCol"][3].asFloat() };
		}
		if (ui.isMember("timerTextCol") && ui["timerTextCol"].size() == 4) {
			timerTextCol = Color{ ui["timerTextCol"][0].asFloat(), ui["timerTextCol"][1].asFloat(),
			ui["timerTextCol"][2].asFloat() ,ui["timerTextCol"][3].asFloat() };
		}
		if (ui.isMember("onCooldownCol") && ui["onCooldownCol"].size() == 4) {
			onCooldownCol = Color{ ui["onCooldownCol"][0].asFloat(), ui["onCooldownCol"][1].asFloat(),
			ui["onCooldownCol"][2].asFloat() ,ui["onCooldownCol"][3].asFloat() };
		}
		if (ui.isMember("availableCol") && ui["availableCol"].size() == 4) {
			availableCol = Color{ ui["availableCol"][0].asFloat(), ui["availableCol"][1].asFloat(),
			ui["availableCol"][2].asFloat() ,ui["availableCol"][3].asFloat() };
		}
	}
	ifs.close();
}

void PetManager::LoadPetData()
{
	std::ifstream ifs{ "Assets/Data/pets.json", std::ios_base::binary };
	if (!ifs.is_open()) return;

	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;

	if (!Json::parseFromStream(builder, ifs, &root, &errs) || !root["pets"].isArray()) {
		std::cout << "LoadEnemyDefs: parse failed: " << errs << "\n";
		return;
	}

	if (!root.isObject() || !root.isMember("pets") || !root["pets"].isArray()) {
		std::cout << "Pet data: missing/invalid 'pets' array\n";
		ifs.close();
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
			for (Json::Value const& m : *passive) {
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
		//Load damage type array
		if (v.findArray("dmgTypes")) {
			for (Json::Value const& m : v["dmgTypes"]) {
				//Each value should be a mod
				pd.dmgTypes.push_back(ParseDmgTypeFromStr(m.asCString()));
			}
		}
		else pd.dmgTypes.push_back(DAMAGE_TYPE::MAGICAL); //By default, add something
		//Rarity scalings (array of numbers)
		if (v.findArray("rarityScaling")) {
			Json::Value const& scales{ v["rarityScaling"] };
			for (int i{}; i < 6; ++i) {
				pd.rarityScaling[i] = scales[i].asFloat();
			}
		}
		pd.skillCooldown = v.get("skillCooldown", 0).asFloat();
		pd.skillDesc = v.get("skillDesc", "").asString();
		if (v.findArray("textures") && v["textures"].size()) {
			for (Json::Value const& m : v["textures"]) {
				pd.textures.push_back(m.asString());
			}
		}
		else {
			pd.textures.push_back("");
		}
		pd.passive.SetIcon(pd.textures.at(0)); //Set passive icon to same tex as pet
		if (v.findArray("skillElements")) {
			for (Json::Value const& m : v["skillElements"]) {
				pd.skillElements.push_back(static_cast<Elements::ELEMENT_TYPE>(m.asInt()));
			}
		}
		if (pd.skillElements.empty()) {
			pd.skillElements.push_back(Elements::ELEMENT_TYPE::NONE);
		}

		//(Optional) Extra status effect stuff
		if (v.findArray("effects")) {
			//Load mods for passive
			Json::Value const* effects{ v.findArray("effects") };
			for (Json::Value const& se : *effects) {
				//Each value should be a mod
				pd.extraEffects.push_back(StatEffects::StatusEffect::ParseFromJson(se));
			}
		}

		if (v.isMember("extra")) {
			Json::Value const& extras{ v["extra"] };
			Json::Value::Members extra_members{ extras.getMemberNames() };
			for (Json::Value::Members::iterator it = extra_members.begin(); it != extra_members.end(); ++it) {
				const std::string& key = *it;
				Json::Value const& value = extras[key];
				pd.extra[key] = value.asString();
			}
		}

		petData[pd.id] = pd;
	}

	ownedPets.clear();
	LoadInventoryCounts(ownedPets);
	ifs.close();
}

void PetManager::CreatePet()
{
	if (selectedPetInfo.first == Pets::PET_TYPE::NONE) {
		return;
	}
	////Set skill ptr, other values
	auto it = petData.find(selectedPetInfo.first);
	if (it == petData.end()) {
		std::cout << "Failed to find pet data\n";
		return;
	}

	switch (selectedPetInfo.first)
	{
	case Pets::PET_1:
		equippedPet = (Pet*)GameObjectManager::GetInstance()->FetchGO(GO_TYPE::PET_1);
		break;
	case Pets::PET_2:
		equippedPet = (Pet*)GameObjectManager::GetInstance()->FetchGO(GO_TYPE::PET_2);
		break;
	case Pets::PET_3:
		equippedPet = (Pet*)GameObjectManager::GetInstance()->FetchGO(GO_TYPE::PET_3);
		break;
	case Pets::PET_4:
		equippedPet = (Pet*)GameObjectManager::GetInstance()->FetchGO(GO_TYPE::PET_4);
		break;
	case Pets::PET_5:
		equippedPet = (Pet*)GameObjectManager::GetInstance()->FetchGO(GO_TYPE::PET_5);
		break;
	case Pets::PET_6:
		equippedPet = (Pet*)GameObjectManager::GetInstance()->FetchGO(GO_TYPE::PET_6);
		break;
	default:
		break;
	}

	equippedPet->Init({}, { 50,50 }, 0, MESH_SQUARE, Collision::COL_CIRCLE, { 25,25 }, CreateBitmask(1, Collision::LAYER::ENEMIES), Collision::LAYER::PET);
	Pets::PetData const& data{ it->second };
	equippedPet->SetData(data, selectedPetInfo.second);
	//equippedPet->GetRenderData().ReplaceTexture(data.textures.at(0).c_str(), 0);
	size_t i{};
	for (std::string const& tex : data.textures) {
		equippedPet->GetRenderData().ReplaceTexture(tex.c_str(), i);
		++i;
	}
	equippedPet->isSet = true;
	equippedPet->SetEnabled(true);
}

void PetManager::SaveInventoryToJSON() {
	Json::Value root;
	Json::Value invArray(Json::arrayValue);

	for (auto const& outer : ownedPets) {
		for (auto const& inner : outer.second) {
			if (inner.second > 0) {
				Json::Value entry;
				entry["petId"] = outer.first;
				entry["rank"] = inner.first;
				entry["count"] = inner.second;
				invArray.append(entry);
			}
		}
	}

	root["inventory"] = invArray;
	std::ofstream file("Assets/Data/pet_inventory.json");
	if (file.is_open()) {
		Json::StyledWriter writer;
		file << writer.write(root);
		file.close();
	}
}

void PetManager::LoadInventoryCounts(std::map<int, std::map<int, int>>& outMap) {
	outMap.clear();
	std::ifstream ifs{ "Assets/Data/pet_inventory.json", std::ios_base::binary };
	if (!ifs.is_open()) return;

	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;

	if (Json::parseFromStream(builder, ifs, &root, &errs)) {
		if (root.isMember("inventory") && root["inventory"].isArray()) {
			for (Json::Value const& v : root["inventory"]) {
				outMap[v.get("petId", 0).asInt()][v.get("rank", 0).asInt()] = v.get("count", 0).asInt();
			}
		}
	}
	ifs.close();
}