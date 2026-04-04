#include "PetState.h"
#include "../Pets/PetInventory.h"
#include "../Pets/PetManager.h"
#include "../Helpers/Vec2Utils.h"
#include "../Helpers/CoordUtils.h"
#include "../Helpers/CollisionUtils.h"
#include "../Helpers/MatrixUtils.h"
#include "../Rendering/RenderingManager.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/ColorUtils.h"
#include "../main.h"
#include "../UI/UIConfig.h"
#include "Related/Gacha.h"
#include "../Music.h"
#include "../GameStates/Related/ShopFunctions.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

static constexpr int GACHA_COST_10 = 1000;
static constexpr int GACHA_COST_100 = 10000;

namespace {

	AEGfxVertexList* squareMesh = nullptr;
	s8 Font = -1, BigFont = -1;

	constexpr float DEFAULT_W = 1600.0f;
	constexpr float DEFAULT_H = 900.0f;

	struct Button
	{
		AEVec2      pos;
		AEVec2      size;
		const char* label;
	};

	struct Title
	{
		AEVec2      pos;
		AEVec2      size;
		const char* label;
	};

	bool gachaNoCoinWarning = false;

	// Main menu buttons for the Pet system
	Button petButtons[] =
	{
		// idx 0 — Pet Inventory  (left column)
		{{ 500.f,  500.f }, { 450.f, 144.f }, "PET INVENTORY" },
		// idx 1 — Gacha          (right column)
		{{ 1100.f, 500.f }, { 450.f, 144.f }, "GACHA"          },
		// idx 2 — Back           (top-left)
		{{ 300.f,  100.f }, { 225.f, 110.f }, "back"           },
	};

	Title titleCfg = { { DEFAULT_W / 2, 100.f }, { 675.f, 110.f }, "PETS" };

	constexpr int PET_BTN_COUNT = sizeof(petButtons) / sizeof(Button);

	float winW, winH, scale;


	AEVec2 DefaultToWorld(float x, float y)
	{
		return {
			(x - DEFAULT_W / 2) * scale,
			(DEFAULT_H / 2 - y) * scale
		};
	}

	// -------------------------------------------------------------------------
	// View state
	// -------------------------------------------------------------------------
	enum class PET_VIEW { MAIN, INVENTORY, GACHA };
	PET_VIEW currentView = PET_VIEW::MAIN;

	bool btnHoverStates[PET_BTN_COUNT] = { false };

	// Gacha animation and font resources
	static GachaAnimation gStateAnim;
	static s8 gachaFont = -1;

	// Grid layout settings for the pet collection screen
	const int    COLUMNS = 6;
	const float  SLOT_SIZE = 140.0f;
	const float  PADDING = 30.0f;
	const AEVec2 GRID_START = { 300.0f, 250.0f };

	struct PetSlot { int petId, rank, count; };
	std::vector<PetSlot> sortedInventory;
	std::vector<bool>    slotHover;

	int selectedIndex = -1;
	Pets::PET_TYPE selectedType = Pets::PET_TYPE::NONE;
	Pets::PET_RANK selectedRank = Pets::COMMON;

	PetManager* petManager = nullptr;

	// UI visuals: Colors associated with pet rarity
	Color GetRarityColor(Pets::PET_RANK rank) {
		switch (rank) {
		case Pets::COMMON:    return CreateColor(200, 200, 200, 255);
		case Pets::UNCOMMON:  return CreateColor(30, 255, 0, 255);
		case Pets::RARE:      return CreateColor(0, 112, 221, 255);
		case Pets::EPIC:      return CreateColor(163, 53, 238, 255);
		case Pets::LEGENDARY: return CreateColor(255, 128, 0, 255);
		case Pets::MYTHICAL:  return CreateColor(255, 0, 0, 255);
		default:              return CreateColor(255, 255, 255, 255);
		}
	}

	// String conversion for rarity ranks
	const char* RankName(Pets::PET_RANK rank) {
		switch (rank) {
		case Pets::COMMON:    return "Common";
		case Pets::UNCOMMON:  return "Uncommon";
		case Pets::RARE:      return "Rare";
		case Pets::EPIC:      return "Epic";
		case Pets::LEGENDARY: return "Legendary";
		case Pets::MYTHICAL:  return "Mythical";
		default:              return "";
		}
	}

	// Refreshes the display list to show newest pets or updated counts
	void RebuildSortedInventory() {
		sortedInventory.clear();
		auto const& inv = petManager->GetInventory();
		for (auto const& outer : inv)
			for (auto const& inner : outer.second)
				if (inner.second > 0)
					sortedInventory.push_back({ outer.first, inner.first, inner.second });

		// Keep lower rarities at the top, then sort by ID
		std::sort(sortedInventory.begin(), sortedInventory.end(),
			[](PetSlot const& a, PetSlot const& b) {
				return (a.rank != b.rank) ? (a.rank < b.rank) : (a.petId < b.petId);
			});
		slotHover.assign(sortedInventory.size(), false);
	}

} 

// =============================================================================
// LoadState
// =============================================================================
void PetState::LoadState()
{
	squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);
	gachaFont = AEGfxCreateFont(SECONDARY_FONT_PATH, 32);
	petManager = PetManager::GetInstance();
	EnsureOverlayMesh();
}

// =============================================================================
// InitState
// =============================================================================
void PetState::InitState()
{
	std::cout << "Pet state enter\n";
	AEGfxFontSystemStart();
	Font = AEGfxCreateFont(PRIMARY_FONT_PATH, 30);
	BigFont = AEGfxCreateFont(PRIMARY_FONT_PATH, 75);
	winW = static_cast<float>(AEGfxGetWinMaxX());
	winH = static_cast<float>(AEGfxGetWinMaxY());
	scale = (winW * 2 / DEFAULT_W) < (winH * 2 / DEFAULT_H)
		? (winW * 2 / DEFAULT_W) : (winH * 2 / DEFAULT_H);

	petManager->Init();

	currentView = PET_VIEW::MAIN;
	selectedIndex = -1;
	selectedType = Pets::PET_TYPE::NONE;
	selectedRank = Pets::COMMON;
	gStateAnim.Reset();

	for (int i = 0; i < PET_BTN_COUNT; ++i) btnHoverStates[i] = false;

	RebuildSortedInventory();

	// Check if a pet is already equipped so we can highlight it in the UI
	for (int idx = 0; idx < (int)sortedInventory.size(); ++idx) {
		auto t = static_cast<Pets::PET_TYPE>(sortedInventory[idx].petId);
		auto rk = static_cast<Pets::PET_RANK>(sortedInventory[idx].rank);
		if (petManager->GetEquippedType() == t && petManager->GetEquippedRank() == rk) {
			selectedIndex = idx;
			selectedType = t;
			selectedRank = rk;
		}
	}
}

// =============================================================================
// Update
// =============================================================================
void PetState::Update(double dt)
{
	// ---- GACHA VIEW ----------------------------------------------------------
	if (currentView == PET_VIEW::GACHA)
	{
		// Back to main pet menu
		if (AEInputCheckTriggered(AEVK_ESCAPE)) {
			bgm.StopGacha(0.2f);
			gStateAnim.Reset();
			currentView = PET_VIEW::MAIN;
			gachaNoCoinWarning = false;
			for (int i = 0; i < PET_BTN_COUNT; ++i) btnHoverStates[i] = false;
			return;
		}

		// Input handling for gacha interaction
		bool openPressed = AEInputCheckTriggered(0x4F);
		bool skipPressed = AEInputCheckTriggered(AEVK_SPACE);
		bool pull10 = AEInputCheckTriggered(0x52);
		bool pull100 = AEInputCheckTriggered(0x54);

		if (gStateAnim.phase == GACHA_PHASE::DONE) {
			petManager->SaveInventoryToJSON();
			RebuildSortedInventory();

			int coins = ShopFunctions::GetInstance()->getMoney();
			if (pull10) {
				if (coins >= GACHA_COST_10) {
					gachaNoCoinWarning = false;
					ShopFunctions::GetInstance()->addMoney(-GACHA_COST_10);
					bgm.PlayGacha();
					BeginGachaOverlay(gStateAnim, 10, 0.1f, 0.8f, 0.3f);
				}
				else {
					gachaNoCoinWarning = true;
					std::cout << "[Gacha] Not enough coins for x10 (need "
						<< GACHA_COST_10 << ", have " << coins << ")\n";
				}
			}
			else if (pull100) {
				if (coins >= GACHA_COST_100) {
					gachaNoCoinWarning = false;
					ShopFunctions::GetInstance()->addMoney(-GACHA_COST_100);
					bgm.PlayGacha();
					BeginGachaOverlay(gStateAnim, 100, 0.1f, 1.2f, 0.2f);
				}
				else {
					gachaNoCoinWarning = true;
					std::cout << "[Gacha] Not enough coins for x100 (need "
						<< GACHA_COST_100 << ", have " << coins << ")\n";
				}
			}
		}

		UpdateGachaOverlay(gStateAnim, static_cast<float>(dt), skipPressed, openPressed);
		if (gStateAnim.isFinished) gStateAnim.phase = GACHA_PHASE::DONE;
		return;
	}

	// ---- INVENTORY VIEW ------------------------------------------------------
	if (currentView == PET_VIEW::INVENTORY)
	{
		if (AEInputCheckTriggered(AEVK_ESCAPE)) {
			currentView = PET_VIEW::MAIN;
			for (int i = 0; i < PET_BTN_COUNT; ++i) btnHoverStates[i] = false;
			return;
		}

		// mouse interaction for selecting pets
		for (int index = 0; index < (int)sortedInventory.size(); ++index) {
			PetSlot const& slot = sortedInventory[index];
			int row = index / COLUMNS;
			int col = index % COLUMNS;

			AEVec2 worldPos = DefaultToWorld(
				GRID_START.x + col * (SLOT_SIZE + PADDING),
				GRID_START.y + row * (SLOT_SIZE + PADDING)
			);
			float sSize = SLOT_SIZE * scale;

			bool isHovered = IsCursorOverWorld(worldPos, sSize, sSize, true);
			if (index < (int)slotHover.size()) {
				if (isHovered && !slotHover[index])
					bgm.PlayUIHover();
				slotHover[index] = isHovered;
			}

			// Toggle equip/unequip on click
			if (isHovered && AEInputCheckTriggered(AEVK_LBUTTON)) {
				bgm.PlayUIClick();
				auto clickedType = static_cast<Pets::PET_TYPE>(slot.petId);
				auto clickedRank = static_cast<Pets::PET_RANK>(slot.rank);

				if (selectedIndex == index) {
					selectedIndex = -1;
					selectedType = Pets::PET_TYPE::NONE;
					selectedRank = Pets::COMMON;
					petManager->SetPet(Pets::PET_TYPE::NONE, Pets::COMMON);
					std::cout << "[PetState] Pet deselected.\n";
				}
				else {
					selectedIndex = index;
					selectedType = clickedType;
					selectedRank = clickedRank;
					petManager->SetPet(clickedType, clickedRank);
					std::cout << "[PetState] Selected pet id=" << slot.petId
						<< " rank=" << slot.rank << "\n";
				}
			}
		}
		return;
	}

	// ---- MAIN VIEW -------------------------
	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		GameStateManager::GetInstance()->SetNextGameState("MainMenuState", true, true);
		return;
	}

	for (int i = 0; i < PET_BTN_COUNT; ++i)
	{
		AEVec2 worldPos = DefaultToWorld(petButtons[i].pos.x, petButtons[i].pos.y);
		AEVec2 worldSize = { petButtons[i].size.x * scale, petButtons[i].size.y * scale };

		bool buttonHover = IsCursorOverWorld(worldPos, worldSize.x, worldSize.y, true);

		if (buttonHover && !btnHoverStates[i])
			bgm.PlayUIHover();
		btnHoverStates[i] = buttonHover;

		if (buttonHover && AEInputCheckTriggered(AEVK_LBUTTON)) {
			bgm.PlayUIClick();
			switch (i) {
			case 0: // Pet Inventory
				currentView = PET_VIEW::INVENTORY;
				RebuildSortedInventory();
				break;
			case 1: // Gacha: checks for enough coin before initiating animation
			{
				currentView = PET_VIEW::GACHA;
				bgm.StopGameplayBGM();
				int coins = ShopFunctions::GetInstance()->getMoney();
				if (coins >= GACHA_COST_10) {
					gachaNoCoinWarning = false;
					ShopFunctions::GetInstance()->addMoney(-GACHA_COST_10);
					bgm.PlayGacha();
					BeginGachaOverlay(gStateAnim, 10, 0.6f, 1.2f, 0.3f);
				}
				else {
					gachaNoCoinWarning = true;
					gStateAnim.Reset();
					gStateAnim.phase = GACHA_PHASE::DONE;
					std::cout << "[Gacha] Not enough coins (need " << GACHA_COST_10 << ")\n";
				}
				break;
			}
			case 2: // Back
				GameStateManager::GetInstance()->SetNextGameState("MainMenuState", true, true);
				break;
			}
			std::cout << "Clicked Pet Button: " << petButtons[i].label << "\n";
		}
	}
}

// =============================================================================
// Draw
// =============================================================================
void PetState::Draw()
{
	// ---- GACHA VIEW ----------------------------------------------------------
	if (currentView == PET_VIEW::GACHA)
	{
		AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
		AEGfxStart();
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		DrawGachaOverlay(gStateAnim, gachaFont);

		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

		// Show currency while pulling
		int    coins = ShopFunctions::GetInstance()->getMoney();
		AEVec2 coinLabelPos = DefaultToWorld(1500.f, 50.f);
		DrawAEText(Font, "COINS:", coinLabelPos, scale, CreateColor(255, 215, 0, 255), TEXT_MIDDLE);
		char coinBuf[32];
		snprintf(coinBuf, sizeof(coinBuf), "%d", coins);
		AEVec2 coinAmtPos = DefaultToWorld(1500.f, 95.f);
		DrawAEText(Font, coinBuf, coinAmtPos, scale, CreateColor(255, 215, 0, 255), TEXT_MIDDLE);

		if (gachaNoCoinWarning) {
			AEVec2 warnPos = DefaultToWorld(DEFAULT_W * 0.5f, DEFAULT_H - 220.f);
			DrawAEText(Font, "NOT ENOUGH COINS!", warnPos, scale * 2.0f,
				CreateColor(255, 60, 60, 255), TEXT_MIDDLE);
		}
		return;
	}

	// ---- INVENTORY VIEW ------------------------------------------------------
	if (currentView == PET_VIEW::INVENTORY)
	{
		AEGfxSetBackgroundColor(0.15f, 0.15f, 0.15f);
		AEGfxStart();
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);

		// Render the header bar
		{
			AEVec2  invTitlePos = DefaultToWorld(DEFAULT_W / 2, 100.f);
			AEVec2  invTitleSize = { 675.f * scale, 110.f * scale };
			AEMtx33 mtx;
			GetTransformMtx(mtx, invTitlePos, 0.0f, invTitleSize);
			AEGfxSetTransform(mtx.m);
			AEGfxSetColorToMultiply(0.75f, 0.75f, 0.75f, 1.0f);
			AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
			AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
			DrawAEText(BigFont, "PET INVENTORY", invTitlePos, scale, CreateColor(10, 10, 10, 255), TEXT_MIDDLE);
			AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		}

		auto const& petDataMap = petManager->GetPetDataMap();

		// Draw the pet grid
		for (int index = 0; index < (int)sortedInventory.size(); ++index) {
			PetSlot const& slot = sortedInventory[index];
			int row = index / COLUMNS;
			int col = index % COLUMNS;

			AEVec2 worldPos = DefaultToWorld(
				GRID_START.x + col * (SLOT_SIZE + PADDING),
				GRID_START.y + row * (SLOT_SIZE + PADDING)
			);
			AEVec2 worldSize = { SLOT_SIZE * scale, SLOT_SIZE * scale };

			bool isSelected = (index == selectedIndex);
			bool hover = (index < (int)slotHover.size()) ? slotHover[index] : false;

			AEMtx33 mtx;

			// Highlight the currently active pet with a gold border
			if (isSelected) {
				AEVec2 borderSize = { worldSize.x + 8.f * scale, worldSize.y + 8.f * scale };
				GetTransformMtx(mtx, worldPos, 0.0f, borderSize);
				AEGfxSetTransform(mtx.m);
				AEGfxSetColorToMultiply(1.0f, 0.85f, 0.0f, 1.0f);
				AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
			}

			// Render the box background for each slot
			GetTransformMtx(mtx, worldPos, 0.0f, worldSize);
			AEGfxSetTransform(mtx.m);
			if (isSelected)      AEGfxSetColorToMultiply(0.5f, 0.45f, 0.1f, 1.0f);
			else if (hover)      AEGfxSetColorToMultiply(0.4f, 0.4f, 0.4f, 1.0f);
			else                  AEGfxSetColorToMultiply(0.2f, 0.2f, 0.2f, 1.0f);
			AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

			// Overlay pet details (Name, quantity, rarity color)
			auto it = petDataMap.find(static_cast<Pets::PET_TYPE>(slot.petId));
			if (it != petDataMap.end()) {
				AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
				Color rarityColor = GetRarityColor(static_cast<Pets::PET_RANK>(slot.rank));

				DrawAEText(Font, it->second.name.c_str(), worldPos, scale, rarityColor, TEXT_MIDDLE);

				AEVec2 countPos = { worldPos.x, worldPos.y - 25.0f * scale };
				std::string countStr = "x" + std::to_string(slot.count);
				DrawAEText(Font, countStr.c_str(), countPos, scale * 0.8f,
					CreateColor(255, 255, 255, 255), TEXT_MIDDLE);

				if (isSelected) {
					AEVec2 labelPos = { worldPos.x, worldPos.y + 40.0f * scale };
					DrawAEText(Font, "EQUIPPED", labelPos, scale * 0.75f,
						CreateColor(255, 220, 0, 255), TEXT_MIDDLE);
				}
				AEGfxSetRenderMode(AE_GFX_RM_COLOR);
			}
		}

		// Tooltip: show the pet's ability description when selected
		if (selectedIndex != -1) {
			auto it = petManager->GetPetDataMap().find(selectedType);
			if (it != petManager->GetPetDataMap().end()) {
				DrawAETextbox(Font, it->second.skillDesc,
					AEVec2{ GRID_START.x + SLOT_SIZE, -DEFAULT_H * 0.3f },
					300.f, 1.f, 0.02f, Color{ 255, 255, 255, 255 },
					TEXT_ORIGIN_POS::TEXT_MIDDLE_LEFT, TEXTBOX_ORIGIN_POS::BOTTOM,
					TextboxBgCfg{ AEVec2{ 0.02f, 0 }, Color{}, 255, nullptr, nullptr });
			}
		}

		// Status bar at the bottom for quick reference
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		AEVec2 hintPos = DefaultToWorld(DEFAULT_W * 0.5f, DEFAULT_H - 40.f);
		if (selectedType != Pets::PET_TYPE::NONE) {
			auto it = petDataMap.find(selectedType);
			if (it != petDataMap.end()) {
				std::string hint = "Equipped: " + it->second.name + " (" + RankName(selectedRank) + ")  |  [ESC] Back";
				DrawAEText(Font, hint.c_str(), hintPos, scale * 0.85f, CreateColor(255, 220, 0, 255), TEXT_MIDDLE);
			}
		}
		else {
			DrawAEText(Font, "No pet selected  |  [ESC] Back", hintPos,
				scale * 0.85f, CreateColor(180, 180, 180, 255), TEXT_MIDDLE);
		}
		return;
	}

	// ---- MAIN VIEW -----------
	AEGfxSetBackgroundColor(0.2f, 0.2f, 0.2f);
	AEGfxStart();
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);

	// Render Main Menu title box
	{
		AEVec2  titlePos = DefaultToWorld(titleCfg.pos.x, titleCfg.pos.y);
		AEVec2  titleSize = { titleCfg.size.x * scale, titleCfg.size.y * scale };
		AEMtx33 mtx;
		GetTransformMtx(mtx, titlePos, 0.0f, titleSize);
		AEGfxSetTransform(mtx.m);
		AEGfxSetColorToMultiply(0.75f, 0.75f, 0.75f, 1.0f);
		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		DrawAEText(BigFont, titleCfg.label, titlePos, scale, CreateColor(10, 10, 10, 255), TEXT_MIDDLE);
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	}

	// Render current gold amount
	{
		AEVec2 coinLabelPos = DefaultToWorld(1500.f, 50.f);
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		DrawAEText(Font, "COINS:", coinLabelPos, scale, CreateColor(255, 215, 0, 255), TEXT_MIDDLE);
		char coinAmount[64];
		snprintf(coinAmount, sizeof(coinAmount), "%d", ShopFunctions::GetInstance()->getMoney());
		AEVec2 coinAmtPos = DefaultToWorld(1500.f, 95.f);
		DrawAEText(Font, coinAmount, coinAmtPos, scale, CreateColor(255, 215, 0, 255), TEXT_MIDDLE);
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	}

	// Render the primary navigation buttons
	for (int i = 0; i < PET_BTN_COUNT; ++i)
	{
		AEVec2  worldPos = DefaultToWorld(petButtons[i].pos.x, petButtons[i].pos.y);
		AEVec2  worldSize = { petButtons[i].size.x * scale, petButtons[i].size.y * scale };
		bool    hover = IsCursorOverWorld(worldPos, worldSize.x, worldSize.y, true);
		AEMtx33 mtx;

		GetTransformMtx(mtx, worldPos, 0.0f, worldSize);
		AEGfxSetTransform(mtx.m);
		AEGfxSetColorToMultiply(hover ? 0.9f : 0.75f, hover ? 0.9f : 0.75f, hover ? 0.9f : 0.75f, 1.0f);
		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		DrawAEText(Font, petButtons[i].label, worldPos, scale, CreateColor(10, 10, 10, 255), TEXT_MIDDLE);
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	}

	// Cost display for Gacha pulls
	{
		float  bottom = petButtons[1].pos.y + petButtons[1].size.y * 0.5f + 20.f;
		AEVec2 costPos = DefaultToWorld(petButtons[1].pos.x, bottom);
		char   costLabel[64];
		snprintf(costLabel, sizeof(costLabel), "x10: %d coins  |  x100: %d coins",
			GACHA_COST_10, GACHA_COST_100);
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		DrawAEText(Font, costLabel, costPos, scale * 0.75f, CreateColor(255, 215, 0, 255), TEXT_MIDDLE);
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	}
}

// =============================================================================
// ExitState / UnloadState
// =============================================================================
void PetState::ExitState()
{
	std::cout << "Exit pet state\n";
	// save the pet collection when leaving this state
	Pets::SaveInventory(petManager->GetInventory());
	bgm.StopGacha(0.2f);
	gStateAnim.Reset();
}

void PetState::UnloadState()
{
	// Cleanup 
	if (Font >= 0) AEGfxDestroyFont(Font);
	if (BigFont >= 0) AEGfxDestroyFont(BigFont);
	if (gachaFont >= 0) { AEGfxDestroyFont(gachaFont); gachaFont = -1; }

	UnloadGacha();
}