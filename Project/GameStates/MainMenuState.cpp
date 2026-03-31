#include "MainMenuState.h"
#include "../Settings.h"
#include "../Music.h"
#include "../Helpers/Vec2Utils.h"
#include "../helpers/CoordUtils.h"
#include "../Helpers/CollisionUtils.h"
#include "../helpers/MatrixUtils.h"
#include "../RenderingManager.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/ColorUtils.h"
#include "../main.h"
#include "../UIConfig.h"
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
	Button menuButtons[] =
	{
		{{ 200.f, 300.f }, { 235.f, 67.f }, "New Run"},
		{{ 200.f, 400.f }, { 235.f, 67.f }, "Pets"},
		{{ 200.f, 500.f }, { 235.f, 67.f }, "Shop"},
		{{ 200.f, 600.f }, { 235.f, 67.f }, "Settings"},
		{{ 200.f, 700.f }, { 235.f, 67.f }, "Credits"},
		{{ 200.f, 800.f }, { 235.f, 67.f }, "Exit Game"},
		{{ 1400.f, 800.f}, {235.f, 67.f}, "Controls & Help"}
	};
	Title title = { { DEFAULT_W *0.5f, 200.f}, {800,600}, "LOOT RUN"};
	std::string logoTex{ "Assets/sprites/logo.png" };

	constexpr int MENU_BTN_COUNT = sizeof(menuButtons) / sizeof(Button);

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

	// Track previous hover state
	bool btnHoverStates[MENU_BTN_COUNT] = { false };
}

void MainMenuState::LoadState()
{
	squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);

	Font = AEGfxCreateFont(PRIMARY_FONT_PATH, 38);
	BigFont = AEGfxCreateFont(PRIMARY_FONT_PATH, 75);

	bgm.Init();
	Settings::Load(); // load saved volumes and apply to bgm groups immediately
}

void MainMenuState::InitState()
{
	winW = static_cast<float>(AEGfxGetWinMaxX());
	winH = static_cast<float>(AEGfxGetWinMaxY());
	scale = (winW * 2 / DEFAULT_W) < (winH * 2 / DEFAULT_H) ? (winW * 2 / DEFAULT_W) : (winH * 2 / DEFAULT_H); //scale of window compared to default

	// Reset hover tracking when entering state
	for (int i = 0; i < MENU_BTN_COUNT; ++i) btnHoverStates[i] = false;
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
}

void MainMenuState::Update(double /*dt*/)
{
	// Settings popup gets first dibs on input.
	// Returns true if it is open and consumed the frame.
	if (Settings::Update(scale))
		return;

	// ESC quits when popup is not open
	if (AEInputCheckTriggered(AEVK_ESCAPE))
	{
		Terminate();
		return;
	}

	for (int i = 0; i < MENU_BTN_COUNT; ++i)
	{
		AEVec2 worldPos = DefaultToWorld(menuButtons[i].pos.x, menuButtons[i].pos.y);
		AEVec2 worldSize = { menuButtons[i].size.x * scale, menuButtons[i].size.y * scale };

		// boolean check names reserved for input/hover checks
		bool buttonHover = IsCursorOverWorld(worldPos, worldSize.x, worldSize.y, true);
		bool buttonClick = false;

		// Play hover sound only when pointer enters button
		if (buttonHover && !btnHoverStates[i])
			bgm.PlayUIHover();
		btnHoverStates[i] = buttonHover;

		if (buttonHover)
		{
			buttonClick = AEInputCheckTriggered(AEVK_LBUTTON);
			if (buttonClick)
			{
				bgm.PlayUIClick();

				switch (i)
				{
				case 0: GameStateManager::GetInstance()->SetNextGameState("LevelSelectState", true, true); break;
				case 1: GameStateManager::GetInstance()->SetNextGameState("PetState", true, true); break;
				case 2: GameStateManager::GetInstance()->SetNextGameState("ShopState", true, true); break;
				case 3: Settings::Open(); break;
				case 4: GameStateManager::GetInstance()->SetNextGameState("CreditState", true, true); break;
				case 5: Terminate(); break;
				case 6: GameStateManager::GetInstance()->SetNextGameState("GuideState", true, false, false); break;
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

	DrawMesh(GetTransformMtx(titlePos, 0, labelSize), squareMesh, RenderingManager::GetInstance()->LoadTexture(logoTex), 255);
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	// ----------------
	// Draw Buttons
	// ----------------

	//  Menu Buttons
	for (int i = 0; i < MENU_BTN_COUNT; ++i)
	{
		AEVec2 worldPos = DefaultToWorld(
			menuButtons[i].pos.x,
			menuButtons[i].pos.y
		);

		AEVec2 worldSize = MultVec2(
			menuButtons[i].size,
			ToVec2(scale, scale)
		);

		bool hover = IsCursorOverWorld(worldPos, worldSize.x, worldSize.y, true);

		AEMtx33 _mtx;
		GetTransformMtx(_mtx, worldPos, 0.f, worldSize);
		AEGfxSetTransform(_mtx.m);

		AEGfxSetColorToMultiply(
			hover ? 0.9f : 0.75f,
			hover ? 0.9f : 0.75f,
			hover ? 0.9f : 0.75f,
			1.0f
		);

		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

		DrawAEText(
			Font, menuButtons[i]. label, worldPos, scale,
			CreateColor(10, 10, 10, 255),
			TEXT_MIDDLE
		);
	}

	// Settings popup (drawn on top of everything)
	Settings::Draw(Font, BigFont, scale);
}
