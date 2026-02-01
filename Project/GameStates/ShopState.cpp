#include "ShopState.h"
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
		bool hasSideButtons;
	};

	struct Title
	{
		AEVec2 pos;
		AEVec2 size;
		const char* label;
	};

	Button shopButtons[] =
	{
		{{ 500.f, 300.f }, { 450.f, 144.f }, "Damage", true},
		{{ 500.f, 500.f }, { 450.f, 144.f }, "Attack Speed", true},
		{{ 500.f, 700.f }, { 450.f, 144.f }, "Move Speed", true},
		{{ 1100.f, 300.f }, { 450.f, 144.f }, "Health", true},
		{{ 1100.f, 500.f }, { 450.f, 144.f }, "Dodge", true},
		{{ 1100.f, 700.f }, { 450.f, 144.f }, "placeholder", true},
		{{ 300.f, 100.f }, { 225.f, 110.f }, "back", false},
		{{ 1300.f, 100.f }, { 225.f, 110.f }, "REFUND", false},
		{{ 915.f, 830.f }, { 80.f, 60.f }, "x1", false},
		{{ 1038.f, 830.f }, { 80.f, 60.f }, "x10", false},
		{{ 1162.f, 830.f }, { 80.f, 60.f }, "x25", false},
		{{ 1285.f, 830.f }, { 80.f, 60.f }, "x50", false},

	};

	Title title = { { DEFAULT_W / 2, 100.f }, { 675.f, 110.f }, "SHOP" };

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

	void DrawSideButtons(const Button& button)
	{
		// Calculate world position for the main button
		AEVec2 worldPos = DefaultToWorld(button.pos.x, button.pos.y);

		// Define sizes: side buttons match the height of the main button
		AEVec2 sideSize = { 50.0f * scale, button.size.y * scale };
		float sideOffset = (button.size.x / 2.0f) * scale - sideSize.x / 2;

		AEMtx33 mtx;

		// --- Minus Button (Red) ---
		AEVec2 minusPos = { worldPos.x - sideOffset, worldPos.y };
		GetTransformMtx(mtx, minusPos, 0.0f, sideSize);
		AEGfxSetTransform(mtx.m);
		AEGfxSetColorToMultiply(0.9f, 0.3f, 0.3f, 1.0f); // Red
		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
		DrawAEText(Font, "-", minusPos, scale, CreateColor(255, 255, 255, 255), TEXT_MIDDLE);

		// --- Plus Button (Green) ---
		AEVec2 plusPos = { worldPos.x + sideOffset, worldPos.y };
		GetTransformMtx(mtx, plusPos, 0.0f, sideSize);
		AEGfxSetTransform(mtx.m);
		AEGfxSetColorToMultiply(0.3f, 0.9f, 0.3f, 1.0f); // Green
		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
		DrawAEText(Font, "+", plusPos, scale, CreateColor(255, 255, 255, 255), TEXT_MIDDLE);
	}

	AEAudioGroup buttonGroup;
	AEAudio hoverSound;
	AEAudio clickSound;

	// Track previous hover state
	bool btnHoverStates[SHOP_BTN_COUNT] = { false };
}

void ShopState::LoadState()
{
	squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);

	buttonGroup = AEAudioCreateGroup();
	hoverSound = AEAudioLoadSound("Assets/Audio/MOUSETRAP_GEN-HDF-17767.wav");
	clickSound = AEAudioLoadSound("Assets/Audio/MOUSETRAP_GEN-HDF-17766.wav");
}

void ShopState::InitState()
{
	std::cout << "Shop state enter\n";
	AEGfxFontSystemStart();
	Font = AEGfxCreateFont("Assets/Exo2-Regular.ttf", 38);
	BigFont = AEGfxCreateFont("Assets/Exo2-Regular.ttf", 75);
	winW = static_cast<float>(AEGfxGetWinMaxX());
	winH = static_cast<float>(AEGfxGetWinMaxY());
	scale = (winW * 2 / DEFAULT_W) < (winH * 2 / DEFAULT_H) ? (winW * 2 / DEFAULT_W) : (winH * 2 / DEFAULT_H);

	// Reset hover tracking when entering state
	for (int i = 0; i < SHOP_BTN_COUNT; ++i) btnHoverStates[i] = false;
}

void ShopState::Update(double dt)
{
	for (int i = 0; i < SHOP_BTN_COUNT; ++i)
	{
		AEVec2 worldPos = DefaultToWorld(shopButtons[i].pos.x, shopButtons[i].pos.y);
		AEVec2 worldSize = { shopButtons[i].size.x * scale, shopButtons[i].size.y * scale };

		bool buttonHover = IsCursorOver(worldPos, worldSize.x, worldSize.y);

		if (buttonHover && !btnHoverStates[i]) {
			AEAudioPlay(hoverSound, buttonGroup, 0.2f, 0.7f, 0);
		}
		btnHoverStates[i] = buttonHover;

		// Main Button Logic
		if (buttonHover && AEInputCheckTriggered(AEVK_LBUTTON)) {
			AEAudioPlay(clickSound, buttonGroup, 0.6f, 0.6f, 0);
			switch (i) {
			case 0: //damage
				break;
			case 1: //attack speed
				break;
			case 2: //move speed
				break;
			case 3: //health
				break;
			case 4: //dodge
				break;
			case 5: //placeholder
				break;
			case 6: //back
				GameStateManager::GetInstance()
					->SetNextGameState("MainMenuState", true, true);
				break;
			case 7: //refund
				break;
			case 8: //x1
				break;
			case 9: //x10
				break;
			case 10: //x25
				break;
			case 11: //x50
				break;
			}
			std::cout << "Clicked Shop Button: " << shopButtons[i].label << std::endl;
		}

		// Side Buttons Logic (Plus and Minus)
		if (shopButtons[i].hasSideButtons) {
			float sideBtnSize = 60.0f * scale;
			float offset = (shopButtons[i].size.x / 2.0f + 50.0f) * scale;

			// Minus Button Check
			AEVec2 minusPos = { worldPos.x - offset, worldPos.y };
			if (IsCursorOver(minusPos, sideBtnSize, sideBtnSize) && AEInputCheckTriggered(AEVK_LBUTTON)) {
				AEAudioPlay(clickSound, buttonGroup, 0.6f, 0.6f, 0);
				std::cout << "Decreased " << shopButtons[i].label << std::endl;
			}

			// Plus Button Check
			AEVec2 plusPos = { worldPos.x + offset, worldPos.y };
			if (IsCursorOver(plusPos, sideBtnSize, sideBtnSize) && AEInputCheckTriggered(AEVK_LBUTTON)) {
				AEAudioPlay(clickSound, buttonGroup, 0.6f, 0.6f, 0);
				std::cout << "Increased " << shopButtons[i].label << std::endl;
			}
		}
	}
}

void ShopState::Draw()
{
	AEGfxSetBackgroundColor(0.2f, 0.2f, 0.2f);
	AEGfxStart();
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);

	// ----------------
	// Draw Title
	// ----------------

	AEVec2 titlePos = DefaultToWorld(title.pos.x, title.pos.y);
	AEVec2 labelSize = { title.size.x * scale,title.size.y * scale };
	AEMtx33 mtx;
	GetTransformMtx(mtx, titlePos, 0.0f, labelSize);
	AEGfxSetTransform(mtx.m);
	AEGfxSetColorToMultiply(0.75f, 0.75f, 0.75f, 1.0f);
	AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
	DrawAEText(BigFont, title.label, titlePos, scale, CreateColor(10, 10, 10, 255), TEXT_MIDDLE);

	// ----------------
	// Draw Buttons
	// ----------------

	for (int i = 0; i < SHOP_BTN_COUNT; ++i)
	{
		AEVec2 worldPos = DefaultToWorld(shopButtons[i].pos.x, shopButtons[i].pos.y);
		AEVec2 worldSize = { shopButtons[i].size.x * scale, shopButtons[i].size.y * scale };
		bool hover = IsCursorOver(worldPos, worldSize.x, worldSize.y);

		// Main Button Rect
		GetTransformMtx(mtx, worldPos, 0.0f, worldSize);
		AEGfxSetTransform(mtx.m);
		AEGfxSetColorToMultiply(hover ? 0.9f : 0.75f, hover ? 0.9f : 0.75f, hover ? 0.9f : 0.75f, 1.0f);
		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
		DrawAEText(Font, shopButtons[i].label, worldPos, scale, CreateColor(10, 10, 10, 255), TEXT_MIDDLE);

		// Side Buttons Rendering
		if (shopButtons[i].hasSideButtons) {
			DrawSideButtons(shopButtons[i]);
		}
	}
}

void ShopState::ExitState() {}
void ShopState::UnloadState() {
	if (Font >= 0) AEGfxDestroyFont(Font);
	if (BigFont >= 0) AEGfxDestroyFont(BigFont);
	AEAudioUnloadAudio(hoverSound);
	AEAudioUnloadAudio(clickSound);
	AEAudioUnloadAudioGroup(buttonGroup);
}