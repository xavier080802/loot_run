#include "PetManager.h"
#include "PetSkills.h"
#include "PetInventory.h" 
#include "../RenderingManager.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <json/json.h>
#include "../File/CSV.h"
#include "../UI/UIElement.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/MatrixUtils.h"
#include "../Helpers/CoordUtils.h"
#include "../Helpers/Vec2Utils.h"

/* Flow
1. App executes, loading pet manager and game state -> LinkPlayer
2. In main menu, pet is selected, calling SetPet (can be several times)
3. Game starts -> GameState.Init -> Calls InitPetForGame to apply pet's passive
4. Game ends -> GameState.ExitState -> Clears status effects, so no need to handle that here.
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
	TextOriginPos tooltipAlignment{};
	TextboxOriginPos boxAlignment{};
	bool showTimerUnit{};
}

void PetManager::Init() {
	po = PostOffice::GetInstance();
	po->Register("PetManager", this);
	rm = RenderingManager::GetInstance();

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

	//UI
	LoadUIJSON();
	skillUI = &(new UIElement{ NormToWorld(iconPos) + iconSize * 0.5f, {iconSize, iconSize}, 1, Collision::COL_RECT })
		->SetHoverCallback([this](bool) {showTooltip = true; }); //Flag this frame to show tooltip (due to order of calling, showing tooltip here will render below icon)
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

	//Generate description for passive
	std::stringstream s{ extraDesc };
	Pets::PetData const& d{ equippedPet->GetPetData() };
	//Generate description for the skills
	if (PetHasSkill()) {
		//Cooldown
		s << "\n[Cooldown " << std::setprecision(1) << d.skillCooldown << "s]\n";
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

bool PetManager::PetHasSkill() const
{
	return equippedPet->isSet && equippedPet->GetPetData().PetSkill;
}

// Updated AddNewPet to use JSON inventory logic
bool PetManager::AddNewPet(Pets::PetSaveData const& newPet)
{
	if (ownedPetsMap.size() >= MAX_PETS) return false;

	// We now use IncrementCount which handles the JSON file automatically
	if (IncrementCount(newPet.id, newPet.rank, 1)) {
		// Refresh local cache map
		LoadInventoryCounts(ownedPetsMap);
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
			equippedPet->CastSkill({ player, equippedPet });
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
	if (!equippedPet) return;

	DrawTintedMesh(GetTransformMtx(skillUI->GetPos(), 0, skillUI->GetSize()),
		rm->GetMesh(MESH_SQUARE), rm->LoadTexture(equippedPet->GetPetData().texture),
		equippedPet->IsOnCooldown() ? onCooldownCol : Color{ 255,255,255,255 }, 255);

	//Write cooldown
	if (equippedPet->IsOnCooldown()) {
		DrawAEText(rm->GetFont(), std::to_string((int)equippedPet->GetCDTimer()) + (showTimerUnit ? "s" : ""), skillUI->GetPos(), timerFontSize,
			0, timerTextCol, TEXT_MIDDLE);
	}

	if (showTooltip) {
		ShowPetTooltip();
		showTooltip = false;
	}
}

void PetManager::ShowPetTooltip()
{
	std::string txt{ equippedPet->GetPetData().skillDesc + extraDesc };

	s32 mx{}, my{};
	AEInputGetCursorPosition(&mx, &my);
	AEVec2 mP = ScreenToWorld(AEVec2{ (float)mx, (float)my });

	f32 nw{}, nh{};
	GetAEMultilineTextSize(rm->GetFont(), txt, descFontSize, nw, nh, lineSpace);
	AEVec2 nOffset = GetTextAlignPosNorm(rm->GetFont(), txt, mP, descFontSize, tooltipAlignment);

	//Draw text box
	DrawAETextbox(rm->GetFont(), txt, AEVec2{ (float)mP.x, (float)mP.y },
		AEGfxGetWinMaxX() * 0.4f, descFontSize, lineSpace, textCol, tooltipAlignment, TextboxOriginPos::BOTTOM,
		TextboxBgCfg{ padding, tooltipBgCol, 255, rm->GetMesh(MESH_SQUARE) });
}

//Load UI stuff from JSON
void PetManager::LoadUIJSON()
{
	std::ifstream ifs{ "Assets/Data/ui.json", std::ios_base::binary };

	if (!ifs.is_open()) {
		std::cout << "PET UI JSON FAILED TO OPEN\n";
		return;
	}

	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;

	if (!Json::parseFromStream(builder, ifs, &root, &errs))
	{
		std::cout << "Pet data: parse failed: " << errs << "\n";
		ifs.close();
		return;
	}
	if (!root.isObject() || !root.isMember("pet_ui")) {
		std::cout << "Pet ui: missing/invalid 'pet_ui' object\n";
		ifs.close();
		return;
	}
	root = root["pet_ui"];
	//Load ui values
	descFontSize = root.get("descFontSize", 0.35f).asFloat();
	timerFontSize = root.get("timerFontSize", 0.5f).asFloat();
	if (root["padding"].isArray() && root["padding"].size() == 2) {
		padding.x = root["padding"][0].asFloat();
		padding.y = root["padding"][1].asFloat();
	}
	if (root["pos"].isArray() && root["pos"].size() == 2) {
		iconPos.x = root["pos"][0].asFloat();
		iconPos.y = root["pos"][1].asFloat();
	}
	iconSize = root.get("size", 75).asFloat();
	lineSpace = root.get("lineSpace", 0.05f).asFloat();
	if (root["textCol"].isArray() && root["textCol"].size() == 4) {
		textCol = { root["textCol"][0].asFloat(), root["textCol"][1].asFloat() ,
		root["textCol"][2].asFloat(),root["textCol"][3].asFloat() };
	}
	if (root["boxCol"].isArray() && root["boxCol"].size() == 4) {
		tooltipBgCol = { root["boxCol"][0].asFloat(), root["boxCol"][1].asFloat() ,
		root["boxCol"][2].asFloat(),root["boxCol"][3].asFloat() };
	}
	if (root["timerTextCol"].isArray() && root["timerTextCol"].size() == 4) {
		timerTextCol = { root["timerTextCol"][0].asFloat(), root["timerTextCol"][1].asFloat() ,
		root["timerTextCol"][2].asFloat(),root["timerTextCol"][3].asFloat() };
	}
	if (root["onCooldownCol"].isArray() && root["onCooldownCol"].size() == 4) {
		onCooldownCol = { root["onCooldownCol"][0].asFloat(), root["onCooldownCol"][1].asFloat() ,
		root["onCooldownCol"][2].asFloat(),root["onCooldownCol"][3].asFloat() };
	}
	showTimerUnit = root.get("showTimerUnit", true).asBool();
	tooltipAlignment = ParseTextAlignment(root.get("textAlignment", "TEXT_LOWER_LEFT").asString());
	boxAlignment = ParseTextboxAlignment(root.get("boxAlignment", "BOTTOM").asString());

	ifs.close();
}

//Load Pet data from JSON
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
		ifs.close();
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
	}

	// Pet inventory
	// Updated: Loading via JSON instead of manual CSV loop
	LoadInventoryCounts(ownedPetsMap);

	ifs.close();
}