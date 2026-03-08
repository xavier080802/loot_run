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

	AEVec2 DefaultToWorld(float x, float y) {
		return {
			(x - DEFAULT_W / 2) * scale,
			(DEFAULT_H / 2 - y) * scale
		};
	}

	Color GetRarityColor(Pets::PET_RANK rank) {
		switch (rank) {
		case Pets::COMMON:    return CreateColor(200, 200, 200, 255); // Grey
		case Pets::UNCOMMON:  return CreateColor(30, 255, 0, 255);   // Green
		case Pets::RARE:      return CreateColor(0, 112, 221, 255);  // Blue
		case Pets::EPIC:      return CreateColor(163, 53, 238, 255); // Purple
		case Pets::LEGENDARY: return CreateColor(255, 128, 0, 255);  // Orange
		case Pets::MYTHICAL:  return CreateColor(255, 0, 0, 255);    // Red
		default:              return CreateColor(255, 255, 255, 255);
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
				// Equips the specific ID and Rank
				PetManager::GetInstance()->SetPet(static_cast<Pets::PET_TYPE>(outer.first), static_cast<Pets::PET_RANK>(inner.first));
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
			if (inner.second <= 0) continue; // Only draw owned pets

			int row = index / COLUMNS;
			int col = index % COLUMNS;

			AEVec2 worldPos = DefaultToWorld(
				GRID_START.x + (col * (SLOT_SIZE + PADDING)),
				GRID_START.y + (row * (SLOT_SIZE + PADDING))
			);
			AEVec2 worldSize = { SLOT_SIZE * scale, SLOT_SIZE * scale };

			AEMtx33 mtx;
			GetTransformMtx(mtx, worldPos, 0.0f, worldSize);
			AEGfxSetTransform(mtx.m);
			bool hover = (index < (int)btnHoverStates.size()) ? btnHoverStates[index] : false;
			AEGfxSetColorToMultiply(hover ? 0.4f : 0.2f, hover ? 0.4f : 0.2f, hover ? 0.4f : 0.2f, 1.0f);
			AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

			// Find display name and apply rarity color
			auto it = petDataMap.find(static_cast<Pets::PET_TYPE>(outer.first));
			if (it != petDataMap.end()) {
				Color rarityColor = GetRarityColor(static_cast<Pets::PET_RANK>(inner.first));

				// Draw Pet Name
				DrawAEText(Font, it->second.name.c_str(), worldPos, scale, rarityColor, TEXT_MIDDLE);

				// Draw specific rank count below
				AEVec2 countPos = { worldPos.x, worldPos.y - (25.0f * scale) };
				std::string countStr = "x" + std::to_string(inner.second);
				DrawAEText(Font, countStr.c_str(), countPos, scale * 0.8f, CreateColor(255, 255, 255, 255), TEXT_MIDDLE);
			}
			index++;
		}
	}
}

void PetState::ExitState() {
	// Save memory to disk on exit
	Pets::SaveInventory(PetManager::GetInstance()->GetInventory());
}

void PetState::UnloadState() {
	if (Font >= 0) AEGfxDestroyFont(Font);
	AEAudioUnloadAudio(hoverSound);
	AEAudioUnloadAudio(clickSound);
	AEAudioUnloadAudioGroup(petAudioGroup);
}