#include "LevelSelectState.h"
#include "GameState.h"
#include "../Helpers/Vec2Utils.h"
#include "../helpers/CoordUtils.h"
#include "../Helpers/CollisionUtils.h"
#include "../Helpers/MatrixUtils.h"
#include "../Rendering/RenderingManager.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/ColorUtils.h"
#include "../Music.h"
#include "../main.h"
#include "../UI/UIConfig.h"
#include <iostream>

std::string mapSelected = "Assets/maps/TutorialMap.csv";

namespace {
	float winW = static_cast<float>(AEGfxGetWinMaxX());
	float winH = static_cast<float>(AEGfxGetWinMaxY());

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
	Button levelButtons[] =
	{
		{{ 800.f, 300.f }, { 235.f, 67.f }, "Tutorial"  },
		{{ 800.f, 420.f }, { 235.f, 67.f }, "Normal"    },
		{{ 800.f, 540.f }, { 235.f, 67.f }, "Endless"   },
		{{ 800.f, 720.f }, { 400.f, 80.f }, "Start game"}
	};

	Title title = { { DEFAULT_W / 2, 100.f }, { 675.f, 110.f }, "LEVEL SELECT" };

	constexpr int LEVEL_BTN_COUNT = sizeof(levelButtons) / sizeof(Button);

	float scale;

	AEVec2 DefaultToWorld(float x, float y)
	{
		return {
			(x - DEFAULT_W / 2) * scale,
			(DEFAULT_H / 2 - y) * scale
		};
	}

	// Track previous hover state
	bool btnHoverStates[LEVEL_BTN_COUNT] = { false };
	int selectedBtn = -1;
}

void LevelSelectState::LoadState() {
	std::cout << "[LevelSelect::InitState] mapSelected = " << mapSelected << "\n";
	squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);
	// All audio handled by bgm - no local audio groups needed
}

void LevelSelectState::InitState() {
	std::cout << "Shop state enter\n";
	AEGfxFontSystemStart();
	Font = AEGfxCreateFont(PRIMARY_FONT_PATH, 38);
	BigFont = AEGfxCreateFont(PRIMARY_FONT_PATH, 75);
	winW = static_cast<float>(AEGfxGetWinMaxX());
	winH = static_cast<float>(AEGfxGetWinMaxY());
	scale = (winW * 2 / DEFAULT_W) < (winH * 2 / DEFAULT_H) ? (winW * 2 / DEFAULT_W) : (winH * 2 / DEFAULT_H);
	if (mapSelected == "Assets/maps/TutorialMap.csv")      selectedBtn = 0;
	else if (mapSelected == "Assets/maps/Dungeon.csv")     selectedBtn = 1;
	else if (mapSelected == "Assets/maps/Endless.csv")     selectedBtn = 2;
	else                                              selectedBtn = 0;
}

void LevelSelectState::ExitState() {}

void LevelSelectState::UnloadState() {
	//unload fonts
	if (Font >= 0)
		AEGfxDestroyFont(Font);
	if (BigFont >= 0)
		AEGfxDestroyFont(BigFont);
}

void LevelSelectState::Update(double /*dt*/) {
	if (AEInputCheckTriggered(AEVK_ESCAPE))
	{
		GameStateManager::GetInstance()->SetNextGameState("MainMenuState");
		return;
	}
	for (int i = 0; i < LEVEL_BTN_COUNT; ++i)
	{
		AEVec2 worldPos = DefaultToWorld(
			levelButtons[i].pos.x,
			levelButtons[i].pos.y
		);

		AEVec2 worldSize = {
			levelButtons[i].size.x * scale,
			levelButtons[i].size.y * scale
		};

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
				case 0: //tutorial
					if (selectedBtn == i)
					{
						// Double-click: already selected, start immediately
						GameStateManager::GetInstance()
							->SetNextGameState("GameState", true, true);
						return;
					}
					mapSelected = "Assets/maps/TutorialMap.csv";
					selectedBtn = i;
					std::cout << "tutorial: " << mapSelected << std::endl;
					break;
				case 1: //normal - CSV dungeon + procedural rooms
					if (selectedBtn == i)
					{
						// Double-click: already selected, start immediately
						GameStateManager::GetInstance()
							->SetNextGameState("GameState", true, true);
						return;
					}
					mapSelected = "Assets/maps/Dungeon.csv";
					selectedBtn = i;
					std::cout << "normal: " << mapSelected << std::endl;
					break;
				case 2: //endless - procedural only, no CSV map
					if (selectedBtn == i)
					{
						// Double-click: already selected, start immediately
						GameStateManager::GetInstance()
							->SetNextGameState("GameState", true, true);
						return;
					}
					mapSelected = "Assets/maps/Endless.csv";
					selectedBtn = i;
					std::cout << "endless: " << mapSelected << std::endl;
					break;
				case 3: //start
					if (selectedBtn == -1) {
						mapSelected = "Assets/maps/TutorialMap.csv";
						selectedBtn = 0;
					}
					GameStateManager::GetInstance()
						->SetNextGameState("GameState", true, true);
					break;
				}
			}
		}
	}
}

void LevelSelectState::Draw() {
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

	AEGfxSetTransform(GetTransformMtx(titlePos, 0.0f, labelSize).m);

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
	// Draw mode description under selected button
	// ----------------
	const char* modeDesc = "";
	if (selectedBtn == 0) modeDesc = "Play the tutorial";
	else if (selectedBtn == 1) modeDesc = "Play the normal dungeon mode";
	else if (selectedBtn == 2) modeDesc = "Play the endless mode";

	AEVec2 descPos = DefaultToWorld(DEFAULT_W / 2, 635.f);
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	DrawAEText(Font, modeDesc, descPos, scale * 0.6f,
		CreateColor(200, 200, 200, 255), TEXT_MIDDLE);
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);

	// ----------------
	// Draw Buttons
	// ----------------

	for (int i = 0; i < LEVEL_BTN_COUNT; ++i)
	{
		AEVec2 worldPos = DefaultToWorld(
			levelButtons[i].pos.x,
			levelButtons[i].pos.y
		);

		AEVec2 worldSize = MultVec2(
			levelButtons[i].size,
			ToVec2(scale, scale)
		);

		bool hover = IsCursorOverWorld(worldPos, worldSize.x, worldSize.y, true);
		bool selected = (i == selectedBtn);

		AEGfxSetTransform(GetTransformMtx(worldPos, 0.0f, worldSize).m);

		// Selected has strongest highlight. Hover still shows a lighter highlight.
		float tint = selected ? 1.0f : (hover ? 0.9f : 0.75f);
		i == 3 ? AEGfxSetColorToMultiply(tint * 0.6f, tint * 0.9f, tint * 0.6f, 1.0f) : AEGfxSetColorToMultiply(tint, tint, tint, 1.0f);

		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
		if (i == 3) {
			DrawAEText(Font, levelButtons[i].label, worldPos, scale * 1.5f, Color(10, 30, 30, 255), TEXT_MIDDLE);
		}
		else {
			DrawAEText(Font, levelButtons[i].label, worldPos, scale, Color(10, 10, 10, 255), TEXT_MIDDLE);
		}
	}
}
