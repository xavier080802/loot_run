#include "PetState.h"
#include "../Pets/PetInventory.h"
#include "../Pets/PetManager.h"
#include "../Helpers/Vec2Utils.h"
#include "../Helpers/CoordUtils.h"
#include "../Helpers/CollisionUtils.h"
#include "../Helpers/MatrixUtils.h"
#include "../RenderingManager.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/ColorUtils.h"
#include "../main.h"
#include <iostream>
#include <vector>
#include <string>

namespace {
	AEGfxVertexList* squareMesh = nullptr;
	s8 Font = -1;

	constexpr float DEFAULT_W = 1600.0f;
	constexpr float DEFAULT_H = 900.0f;
	const int COLUMNS = 6;
	const float SLOT_SIZE = 140.0f;
	const float PADDING = 30.0f;
	const AEVec2 GRID_START = { 300.0f, 250.0f };

	float winW, winH, scale;

	AEAudioGroup petAudioGroup;
	AEAudio hoverSound, clickSound;
	std::vector<bool> btnHoverStates;

	// Tracks which inventory slot is currently equipped so we can highlight it
	int selectedIndex = -1;
	// Stores the type/rank of the currently selected pet so we can redraw the label
	Pets::PET_TYPE selectedType = Pets::PET_TYPE::NONE;
	Pets::PET_RANK selectedRank = Pets::COMMON;

	AEVec2 DefaultToWorld(float x, float y) {
		return {
			(x - DEFAULT_W / 2) * scale,
			(DEFAULT_H / 2 - y) * scale
		};
	}

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
}

void PetState::LoadState() {
	squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);
	petAudioGroup = AEAudioCreateGroup();
	hoverSound = AEAudioLoadSound("Assets/Audio/MOUSETRAP_GEN-HDF-17767.wav");
	clickSound = AEAudioLoadSound("Assets/Audio/MOUSETRAP_GEN-HDF-17766.wav");
	Font = AEGfxCreateFont("Assets/Exo2-Regular.ttf", 20);
}

void PetState::InitState() {
	winW = static_cast<float>(AEGfxGetWinMaxX());
	winH = static_cast<float>(AEGfxGetWinMaxY());
	scale = (winW * 2 / DEFAULT_W) < (winH * 2 / DEFAULT_H) ? (winW * 2 / DEFAULT_W) : (winH * 2 / DEFAULT_H);

	PetManager::GetInstance()->Init();

	btnHoverStates.assign(Pets::NUM_PETS * 6, false);

	// Restore highlight if a pet was already equipped from a previous visit
	selectedIndex = -1;
	selectedType = Pets::PET_TYPE::NONE;
	selectedRank = Pets::COMMON;

	// Restore highlight if a pet was already equipped from a previous visit.
	auto const& inventory = PetManager::GetInstance()->GetInventory();
	int idx = 0;
	for (auto const& outer : inventory) {
		for (auto const& inner : outer.second) {
			if (inner.second <= 0) { idx++; continue; }
			Pets::PET_TYPE t = static_cast<Pets::PET_TYPE>(outer.first);
			Pets::PET_RANK rk = static_cast<Pets::PET_RANK>(inner.first);
			// PetManager::GetEquippedType() returns the id stored in equippedPet->GetPetData()
			// only when isSet is true compare type and rank directly
			if (PetManager::GetInstance()->GetEquippedType() == t &&
				PetManager::GetInstance()->GetEquippedRank() == rk)
			{
				selectedIndex = idx;
				selectedType = t;
				selectedRank = rk;
			}
			idx++;
		}
	}
}

void PetState::Update(double dt) {
	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		GameStateManager::GetInstance()->SetNextGameState("MainMenuState", true, true);
		return;
	}

	auto const& inventory = PetManager::GetInstance()->GetInventory();
	int index = 0;

	for (auto const& outer : inventory) {
		for (auto const& inner : outer.second) {
			if (inner.second <= 0) continue;

			int row = index / COLUMNS;
			int col = index % COLUMNS;

			AEVec2 worldPos = DefaultToWorld(
				GRID_START.x + (col * (SLOT_SIZE + PADDING)),
				GRID_START.y + (row * (SLOT_SIZE + PADDING))
			);
			float sSize = SLOT_SIZE * scale;

			bool isHovered = IsCursorOverWorld(worldPos, sSize, sSize, true);

			if (isHovered && !btnHoverStates[index]) {
				AEAudioPlay(hoverSound, petAudioGroup, 0.2f, 0.7f, 0);
			}
			btnHoverStates[index] = isHovered;

			if (isHovered && AEInputCheckTriggered(AEVK_LBUTTON)) {
				AEAudioPlay(clickSound, petAudioGroup, 0.6f, 0.6f, 0);

				Pets::PET_TYPE clickedType = static_cast<Pets::PET_TYPE>(outer.first);
				Pets::PET_RANK clickedRank = static_cast<Pets::PET_RANK>(inner.first);

				// Clicking the already-selected slot deselects (brings no pet)
				if (selectedIndex == index) {
					selectedIndex = -1;
					selectedType = Pets::PET_TYPE::NONE;
					selectedRank = Pets::COMMON;
					PetManager::GetInstance()->SetPet(Pets::PET_TYPE::NONE, Pets::COMMON);
					std::cout << "[PetState] Pet deselected.\n";
				}
				else {
					selectedIndex = index;
					selectedType = clickedType;
					selectedRank = clickedRank;
					PetManager::GetInstance()->SetPet(clickedType, clickedRank);
					std::cout << "[PetState] Selected pet id=" << outer.first
						<< " rank=" << inner.first << "\n";
				}
			}
			index++;
		}
	}
}

void PetState::Draw() {
	AEGfxSetBackgroundColor(0.15f, 0.15f, 0.15f);
	AEGfxStart();
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);

	auto const& inventory = PetManager::GetInstance()->GetInventory();
	auto const& petDataMap = PetManager::GetInstance()->GetPetDataMap();

	int index = 0;
	for (auto const& outer : inventory) {
		for (auto const& inner : outer.second) {
			if (inner.second <= 0) continue;

			int row = index / COLUMNS;
			int col = index % COLUMNS;

			AEVec2 worldPos = DefaultToWorld(
				GRID_START.x + (col * (SLOT_SIZE + PADDING)),
				GRID_START.y + (row * (SLOT_SIZE + PADDING))
			);
			AEVec2 worldSize = { SLOT_SIZE * scale, SLOT_SIZE * scale };

			AEMtx33 mtx;
			bool isSelected = (index == selectedIndex);
			bool hover = (index < (int)btnHoverStates.size()) ? btnHoverStates[index] : false;

			// Selected slot: bright gold border drawn slightly larger behind the slot
			if (isSelected) {
				AEVec2 borderSize = { worldSize.x + 8.f * scale, worldSize.y + 8.f * scale };
				GetTransformMtx(mtx, worldPos, 0.0f, borderSize);
				AEGfxSetTransform(mtx.m);
				AEGfxSetColorToMultiply(1.0f, 0.85f, 0.0f, 1.0f); // gold
				AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
			}

			// Slot background � brighter when hovered or selected
			GetTransformMtx(mtx, worldPos, 0.0f, worldSize);
			AEGfxSetTransform(mtx.m);
			if (isSelected)
				AEGfxSetColorToMultiply(0.5f, 0.45f, 0.1f, 1.0f); // warm tint when selected
			else if (hover)
				AEGfxSetColorToMultiply(0.4f, 0.4f, 0.4f, 1.0f);
			else
				AEGfxSetColorToMultiply(0.2f, 0.2f, 0.2f, 1.0f);
			AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

			// Pet name and rarity
			auto it = petDataMap.find(static_cast<Pets::PET_TYPE>(outer.first));
			if (it != petDataMap.end()) {
				AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
				Color rarityColor = GetRarityColor(static_cast<Pets::PET_RANK>(inner.first));

				DrawAEText(Font, it->second.name.c_str(), worldPos, scale, rarityColor, TEXT_MIDDLE);

				AEVec2 countPos = { worldPos.x, worldPos.y - (25.0f * scale) };
				std::string countStr = "x" + std::to_string(inner.second);
				DrawAEText(Font, countStr.c_str(), countPos, scale * 0.8f, CreateColor(255, 255, 255, 255), TEXT_MIDDLE);

				// "EQUIPPED" label on the selected slot
				if (isSelected) {
					AEVec2 labelPos = { worldPos.x, worldPos.y + (40.0f * scale) };
					DrawAEText(Font, "EQUIPPED", labelPos, scale * 0.75f, CreateColor(255, 220, 0, 255), TEXT_MIDDLE);
				}

				AEGfxSetRenderMode(AE_GFX_RM_COLOR);
			}
			index++;
		}
	}

	// Bottom hint bar
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEVec2 hintPos = DefaultToWorld(DEFAULT_W * 0.5f, DEFAULT_H - 40.0f);
	if (selectedType != Pets::PET_TYPE::NONE) {
		auto it = petDataMap.find(selectedType);
		if (it != petDataMap.end()) {
			std::string hint = "Selected: " + it->second.name + " (" + RankName(selectedRank) + ")  |  [ESC] Back";
			DrawAEText(Font, hint.c_str(), hintPos, scale * 0.85f, CreateColor(255, 220, 0, 255), TEXT_MIDDLE);
		}
	}
	else {
		DrawAEText(Font, "No pet selected  |  [ESC] Back", hintPos, scale * 0.85f, CreateColor(180, 180, 180, 255), TEXT_MIDDLE);
	}
}

void PetState::ExitState() {
	// Save inventory to disk on exit
	Pets::SaveInventory(PetManager::GetInstance()->GetInventory());
}

void PetState::UnloadState() {
	if (Font >= 0) AEGfxDestroyFont(Font);
	AEAudioUnloadAudio(hoverSound);
	AEAudioUnloadAudio(clickSound);
	AEAudioUnloadAudioGroup(petAudioGroup);
}