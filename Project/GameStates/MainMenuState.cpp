#include "MainMenuState.h"
#include "../Helpers/Vec2Utils.h"
#include "../helpers/CoordUtils.h"
#include "../Helpers/CollisionUtils.h"
#include "../helpers/MatrixUtils.h"
#include "../RenderingManager.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/ColorUtils.h"
#include "../main.h"
#include <iostream>

//helpers
namespace {

	AEGfxVertexList* squareMesh = nullptr;
	s8 Font = -1, BigFont = -1;
	constexpr float DEFAULT_W = 1600.0f;
	constexpr float DEFAULT_H = 900.0f;

	struct Button
	{
		AEVec2 pos;
		AEVec2 size;
		const char* label;
	};
	struct Title
	{
		AEVec2 pos;
		AEVec2 size;
		const char* label;
	};
	Button shopButtons[] =
	{
		{{ 200.f, 300.f }, { 235.f, 67.f }, "New Run"},
		{{ 200.f, 400.f }, { 235.f, 67.f }, "Pets"},
		{{ 200.f, 500.f }, { 235.f, 67.f }, "Shop"},
		{{ 200.f, 600.f }, { 235.f, 67.f }, "Settings"},
		{{ 200.f, 700.f }, { 235.f, 67.f }, "Credits"},
		{{ 200.f, 800.f }, { 235.f, 67.f }, "Exit Game"}
	};
	Title title = { { DEFAULT_W / 2, 100.f }, { 675.f, 110.f }, "LOOT RUN" };

	constexpr int SHOP_BTN_COUNT = sizeof(shopButtons) / sizeof(Button);

	float winW;
	float winH;
	float scale;

	AEVec2 DefaultToWorld(float x, float y)
	{
		return {
			(x - DEFAULT_W / 2) * scale,
			(DEFAULT_H / 2 - y) * scale
		};
	}


	AEAudioGroup buttonGroup;
	AEAudio hoverSound;

	AEAudio clickSound;

	// Track previous hover state
	bool btnHoverStates[SHOP_BTN_COUNT] = { false };
}
void MainMenuState::LoadState()
{
	squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);

	buttonGroup = AEAudioCreateGroup();
	hoverSound = AEAudioLoadSound("Assets/Audio/MOUSETRAP_GEN-HDF-17767.wav");
	clickSound = AEAudioLoadSound("Assets/Audio/MOUSETRAP_GEN-HDF-17766.wav");
	Font = AEGfxCreateFont("Assets/Exo2-Regular.ttf", 38);
	BigFont = AEGfxCreateFont("Assets/Exo2-Regular.ttf", 75);
}

void MainMenuState::InitState()
{
	winW = static_cast<float>(AEGfxGetWinMaxX());
	winH = static_cast<float>(AEGfxGetWinMaxY());
	scale = (winW * 2 / DEFAULT_W) < (winH * 2 / DEFAULT_H) ? (winW * 2 / DEFAULT_W) : (winH * 2 / DEFAULT_H); //scale of window compared to default

	// Reset hover tracking when entering state
	for (int i = 0; i < SHOP_BTN_COUNT; ++i) btnHoverStates[i] = false;
}

void MainMenuState::ExitState()
{
}

void MainMenuState::UnloadState()
{
	//unload fonts
	if (Font >= 0)
		AEGfxDestroyFont(Font);
	if (BigFont >= 0)
		AEGfxDestroyFont(BigFont);

	// Unload button audio
	AEAudioUnloadAudio(hoverSound);
	AEAudioUnloadAudio(clickSound);
	AEAudioUnloadAudioGroup(buttonGroup);
}

void MainMenuState::Update(double dt)
{
	if (AEInputCheckTriggered(AEVK_ESCAPE))
	{
		Terminate();
		return;
	}
	for (int i = 0; i < SHOP_BTN_COUNT; ++i)
	{
		AEVec2 worldPos = DefaultToWorld(
			shopButtons[i].pos.x,
			shopButtons[i].pos.y
		);

		AEVec2 worldSize = {
			shopButtons[i].size.x * scale,
			shopButtons[i].size.y * scale
		};

		// boolean check names reserved for input/hover checks
		bool buttonHover = IsCursorOverWorld(worldPos, worldSize.x, worldSize.y, true);
		bool buttonClick = false;

		// Play hover sound only when pointer enters button
		if (buttonHover && !btnHoverStates[i])
		{
			AEAudioPlay(hoverSound, buttonGroup, 0.2f, 0.7f, 0);
		}
		btnHoverStates[i] = buttonHover;

		if (buttonHover)
		{
			buttonClick = AEInputCheckTriggered(AEVK_LBUTTON);
			if (buttonClick)
			{
				AEAudioPlay(clickSound, buttonGroup, 0.6f, 0.6f, 0);

				switch (i)
				{
				case 0: //new game
					GameStateManager::GetInstance()
						->SetNextGameState("GameState", true, true);
					
					break;
				case 1: // pet
					/*GameStateManager::GetInstance()
						->SetNextGameState("PetMenuState", true, true);*/
					break;
				case 2: // shop
					GameStateManager::GetInstance()
						->SetNextGameState("ShopState", true, true);
					break;
				case 3: //settings
					/*GameStateManager::GetInstance()
						->SetNextGameState("SettingsState", true, true);*/
					break;
				case 4: //credits
					/*GameStateManager::GetInstance()
						->SetNextGameState("CreditsState", true, true);*/
					break;
				case 5: //exit game
					Terminate();
					break;
				}
			}
		}
	}
}

void MainMenuState::Draw()
{
	AEGfxSetBackgroundColor(0.2f, 0.2f, 0.2f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);

	// ----------------
	// Draw Title
	// ----------------
	AEVec2 titlePos = DefaultToWorld(
		title.pos.x,
		title.pos.y
	);

	AEVec2 labelSize = {
		title.size.x * scale,
		title.size.y * scale
	};

	AEMtx33 mtx;
	GetTransformMtx(mtx, titlePos, 0.0f, labelSize);
	AEGfxSetTransform(mtx.m);

	AEGfxSetColorToMultiply(
		0.75f,
		0.75f,
		0.75f,
		1.0f
	);

	AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

	DrawAEText(
		BigFont, title.label, titlePos, scale,
		CreateColor(10, 10, 10, 255),
		TEXT_MIDDLE
	);

	// ----------------
	// Draw Buttons
	// ----------------

	for (int i = 0; i < SHOP_BTN_COUNT; ++i)
	{
		AEVec2 worldPos = DefaultToWorld(
			shopButtons[i].pos.x,
			shopButtons[i].pos.y
		);

		AEVec2 worldSize = MultVec2(
			shopButtons[i].size,
			ToVec2(scale, scale)
		);

		bool hover = IsCursorOverWorld(worldPos, worldSize.x, worldSize.y, true);

		AEMtx33 mtx;
		GetTransformMtx(mtx, worldPos, 0.0f, worldSize);
		AEGfxSetTransform(mtx.m);

		AEGfxSetColorToMultiply(
			hover ? 0.9f : 0.75f,
			hover ? 0.9f : 0.75f,
			hover ? 0.9f : 0.75f,
			1.0f
		);

		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

		DrawAEText(
			Font, shopButtons[i]. label, worldPos, scale,
			CreateColor(10, 10, 10, 255),
			TEXT_MIDDLE
		);
	}
}