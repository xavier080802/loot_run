#include "LevelSelectState.h"
#include "GameState.h"
#include "../Helpers/Vec2Utils.h"
#include "../helpers/CoordUtils.h"
#include "../Helpers/CollisionUtils.h"
#include "../helpers/MatrixUtils.h"
#include "../RenderingManager.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/ColorUtils.h"
#include "../main.h"
#include <iostream>

std::string mapSelected = "Assets/TutorialMap.csv";

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
		{{ 800.f, 250.f }, { 235.f, 67.f }, "Tutorial"},
		{{ 800.f, 350.f }, { 235.f, 67.f }, "Dungeon 1"},
		{{ 800.f, 450.f }, { 235.f, 67.f }, "Dungeon 2"},
		{{ 800.f, 550.f }, { 235.f, 67.f }, "Dungeon 3"},
		{{ 800.f, 650.f }, { 235.f, 67.f }, "Openworld"},
		{{ 800.f, 800.f }, { 400.f, 80.f }, "Start game" }
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


	AEAudioGroup buttonGroup;
	AEAudio hoverSound;

	AEAudio clickSound;

	// Track previous hover state
	bool btnHoverStates[LEVEL_BTN_COUNT] = { false };
	int selectedBtn = -1;
}

void LevelSelectState::LoadState() {
	squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);

	buttonGroup = AEAudioCreateGroup();
	hoverSound = AEAudioLoadSound("Assets/Audio/MOUSETRAP_GEN-HDF-17767.wav");
	clickSound = AEAudioLoadSound("Assets/Audio/MOUSETRAP_GEN-HDF-17766.wav");
	
}

void LevelSelectState::InitState() {
	std::cout << "Shop state enter\n";
	AEGfxFontSystemStart();
	Font = AEGfxCreateFont("Assets/Exo2-Regular.ttf", 38);
	BigFont = AEGfxCreateFont("Assets/Exo2-Regular.ttf", 75);
	winW = static_cast<float>(AEGfxGetWinMaxX());
	winH = static_cast<float>(AEGfxGetWinMaxY());
	scale = (winW * 2 / DEFAULT_W) < (winH * 2 / DEFAULT_H) ? (winW * 2 / DEFAULT_W) : (winH * 2 / DEFAULT_H);
}

void LevelSelectState::ExitState() {}

void LevelSelectState::UnloadState() {
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

void LevelSelectState::Update(double dt) {
	if (AEInputCheckTriggered(AEVK_ESCAPE))
	{
		Terminate();
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
				case 0: //tutorial
					mapSelected = "Assets/TutorialMap.csv";
					selectedBtn = i;
					std::cout << "tutorial: " << mapSelected << std::endl;
					break;
				case 1: //level1
					mapSelected = "Assets/Dungeon.csv";
					selectedBtn = i;
					std::cout << "level1: " << mapSelected << std::endl;
					break;
				case 2: //level2
					mapSelected = "Assets/Dungeon.csv";
					selectedBtn = i;
					std::cout << "level2: " << mapSelected << std::endl;
					break;
				case 3: //level3
					mapSelected = "Assets/Dungeon.csv";
					selectedBtn = i;
					std::cout << "level3: " << mapSelected << std::endl;
					break;
				case 4: //endless
					mapSelected = "Assets/Openworld.csv";
					selectedBtn = i;
					std::cout << "endless: " << mapSelected << std::endl;
					break;
				case 5: //start
					if (selectedBtn == -1) {
						mapSelected = "Assets/TutorialMap.csv";
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

		AEMtx33 mtx;
		GetTransformMtx(mtx, worldPos, 0.0f, worldSize);
		AEGfxSetTransform(mtx.m);

		// Selected has strongest highlight. Hover still shows a lighter highlight.
		float tint = selected ? 1.0f : (hover ? 0.9f : 0.75f);
		AEGfxSetColorToMultiply(
			tint,
			tint,
			tint,
			1.0f
		);

		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);

		DrawAEText(
			Font, levelButtons[i].label, worldPos, scale,
			CreateColor(10, 10, 10, 255),
			TEXT_MIDDLE
		);
	}
}