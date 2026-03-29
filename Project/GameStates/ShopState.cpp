#include "ShopState.h"
#include "../ShopFunctions.h"
#include "../Helpers/Vec2Utils.h"
#include "../helpers/CoordUtils.h"
#include "../Helpers/CollisionUtils.h"
#include "../helpers/MatrixUtils.h"
#include "../RenderingManager.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/ColorUtils.h"
#include "../main.h"
#include <iostream>
//#include "../gacha.h"
#include "../Music.h"
#include "../Pets/PetManager.h"
#include "../UIConfig.h"

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
		{{ 1100.f, 500.f }, { 450.f, 144.f }, "Defense", true},
		//{{ 1100.f, 700.f }, { 450.f, 144.f }, "Enter Gacha", false},
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
	enum SideHover { NONE = 0, MINUS, PLUS };
	void DrawSideButtons(const Button& button, SideHover hoverType)
	{
		// Calculate world position for the main button
		AEVec2 worldPos = DefaultToWorld(button.pos.x, button.pos.y);

		// Define sizes: side buttons match the height of the main button
		AEVec2 sideSize = { 60.0f * scale, button.size.y * scale };
		float sideOffset = (button.size.x / 2.0f) * scale - sideSize.x / 2;
		bool pHover = false, mHover = false;
		if (hoverType == PLUS) {
			pHover = true;
		}
		if (hoverType == MINUS) {
			mHover = true;
		}
		AEMtx33 mtx;

		// --- Minus Button (Red) ---
		AEVec2 minusPos = { worldPos.x - sideOffset, worldPos.y };
		GetTransformMtx(mtx, minusPos, 0.0f, sideSize);
		AEGfxSetTransform(mtx.m);
		AEGfxSetColorToMultiply(mHover ? 0.9f : 0.75f, mHover ? 0.3f : 0.2f, mHover ? 0.3f : 0.2f, 1.0f); // Red
		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
		DrawAEText(Font, "-", minusPos, scale, CreateColor(255, 255, 255, 255), TEXT_MIDDLE);

		// --- Plus Button (Green) ---
		AEVec2 plusPos = { worldPos.x + sideOffset, worldPos.y };
		GetTransformMtx(mtx, plusPos, 0.0f, sideSize);
		AEGfxSetTransform(mtx.m);
		AEGfxSetColorToMultiply(pHover ? 0.3f : 0.2f, pHover ? 0.9f : 0.75f, pHover ? 0.3f : 0.2f, 1.0f); // Green
		AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
		DrawAEText(Font, "+", plusPos, scale, CreateColor(255, 255, 255, 255), TEXT_MIDDLE);
	}

	AEAudio hoverSound;
	AEAudio clickSound;

	// Track previous hover state
	bool btnHoverStates[SHOP_BTN_COUNT] = { false };
	//static GachaAnimation gStateAnim;
	//static s8 gachaFont = -1;
	//bool isGachaActive = false;
}

void ShopState::LoadState()
{
	squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);

	hoverSound = AEAudioLoadSound("Assets/Audio/MOUSETRAP_GEN-HDF-17767.wav");
	clickSound = AEAudioLoadSound("Assets/Audio/MOUSETRAP_GEN-HDF-17766.wav");
	//gachaFont = AEGfxCreateFont(SECONDARY_FONT_PATH, 32);
	//EnsureOverlayMesh();
}

void ShopState::InitState()
{
	std::cout << "Shop state enter\n";
	AEGfxFontSystemStart();
	Font = AEGfxCreateFont(PRIMARY_FONT_PATH, 38);
	BigFont = AEGfxCreateFont(PRIMARY_FONT_PATH, 75);
	winW = static_cast<float>(AEGfxGetWinMaxX());
	winH = static_cast<float>(AEGfxGetWinMaxY());
	scale = (winW * 2 / DEFAULT_W) < (winH * 2 / DEFAULT_H) ? (winW * 2 / DEFAULT_W) : (winH * 2 / DEFAULT_H);

	// Reset hover tracking when entering state
	for (int i = 0; i < SHOP_BTN_COUNT; ++i) btnHoverStates[i] = false;
	//isGachaActive = false;
}

void ShopState::Update(double dt)
{
	//if (isGachaActive)
	//{
	//	bool openPressed = AEInputCheckTriggered(AEVK_LBUTTON) || AEInputCheckTriggered(0x4F);
	//	bool skipPressed = AEInputCheckTriggered(AEVK_SPACE);
	//	bool pull10 = AEInputCheckTriggered(AEVK_LBUTTON) || AEInputCheckTriggered(0x52);
	//	bool pull100 = AEInputCheckTriggered(AEVK_T) || AEInputCheckTriggered(0x54);
	//	bool exitPressed = AEInputCheckTriggered(AEVK_ESCAPE);

	//	if (exitPressed) {
	//		isGachaActive = false;
	//		bgm.StopGacha(0.2f);
	//		return;
	//	}

	//	// Main Button Logic
	//	if (gStateAnim.phase == GachaPhase::Done) {
	//		PetManager::GetInstance()->SaveInventoryToJSON();
	//		if (pull10) BeginGachaOverlay(gStateAnim, 10, 0.1f, 0.8f, 0.3f);
	//		else if (pull100) BeginGachaOverlay(gStateAnim, 100, 0.1f, 1.2f, 0.2f);
	//	}

	//	UpdateGachaOverlay(gStateAnim, static_cast<float>(dt), skipPressed, openPressed);
	//	if (gStateAnim.isFinished) gStateAnim.phase = GachaPhase::Done;
	//}
	//else
	//{
		for (int i = 0; i < SHOP_BTN_COUNT; ++i)
		{
			AEVec2 worldPos = DefaultToWorld(shopButtons[i].pos.x, shopButtons[i].pos.y);
			AEVec2 worldSize = { shopButtons[i].size.x * scale, shopButtons[i].size.y * scale };

			// boolean check names reserved for input/hover checks
			bool buttonHover = IsCursorOverWorld(worldPos, worldSize.x, worldSize.y, true);
			bool buttonClick = false;

			// Play hover sound only when pointer enters button
			if (buttonHover && !btnHoverStates[i])
			{
				AEAudioPlay(hoverSound, bgm.uiGroup, 0.2f, 0.7f, 0);
			}
			btnHoverStates[i] = buttonHover;

			if (buttonHover)
			{
				buttonClick = AEInputCheckTriggered(AEVK_LBUTTON);
				if (buttonClick)
				{
					AEAudioPlay(clickSound, bgm.uiGroup, 0.6f, 0.6f, 0);
					switch (i) {
					case 0: // Damage
						break;
					case 1: // Attack Speed
						break;
					case 2: // Move Speed
						break;
					case 3: // Health
						break;
					case 4: // Defense
						break;
					case 5: // Gacha Trigger
						//isGachaActive = true;
						//bgm.StopGameplayBGM();
						//bgm.PlayGacha();
						//BeginGachaOverlay(gStateAnim, 10, 0.6f, 1.2f, 0.3f);
						//break;
					case 6: //back
						GameStateManager::GetInstance()
							->SetNextGameState("MainMenuState", true, true);
						break;
					case 7: // Refund
						ShopFunctions::GetInstance()->sellAllShopUpgrades();
						break;
					case 8: //x1
						ShopFunctions::GetInstance()->setPurchaseMultiplier(1);
						break;
					case 9: //x10
						ShopFunctions::GetInstance()->setPurchaseMultiplier(10);
						break;
					case 10: //x25
						ShopFunctions::GetInstance()->setPurchaseMultiplier(25);
						break;
					case 11: //x50
						ShopFunctions::GetInstance()->setPurchaseMultiplier(50);
						break;
					}
					std::cout << "Clicked Shop Button: " << shopButtons[i].label << std::endl;
				}
			}
			// Side Buttons Logic (Plus and Minus)
			if (shopButtons[i].hasSideButtons) {
				float sideBtnSize = 60.0f * scale;
				float sideOffset = (shopButtons[i].size.x / 2.0f) * scale - sideBtnSize / 2;

				// Identify which stat this specific button controls
				STAT_TYPE currentStat =
					(i == 0) ? STAT_TYPE::ATT :
					(i == 1) ? STAT_TYPE::ATT_SPD :
					(i == 2) ? STAT_TYPE::MOVE_SPD :
					(i == 3) ? STAT_TYPE::MAX_HP : STAT_TYPE::DEF;

				// Minus Button Check
				AEVec2 minusPos = { worldPos.x - sideOffset, worldPos.y };
				if (IsCursorOverWorld(minusPos, sideBtnSize, shopButtons[i].size.y, true)) {
					if (AEInputCheckTriggered(AEVK_LBUTTON)) {
						AEAudioPlay(clickSound, bgm.uiGroup, 0.6f, 0.6f, 0);
						for (size_t x = 0; x < ShopFunctions::GetInstance()->getPurchaseMultiplier(); x++)
						{
							ShopFunctions::GetInstance()->sellShopUpgrade(currentStat);
						}
						std::cout << "Decreased " << shopButtons[i].label << " to " << ShopFunctions::GetInstance()->getStatBonus(currentStat) << std::endl;
					}
				}

				// Plus Button Check
				AEVec2 plusPos = { worldPos.x + sideOffset, worldPos.y };
				if (IsCursorOverWorld(plusPos, sideBtnSize, shopButtons[i].size.y, true)) {
					if (AEInputCheckTriggered(AEVK_LBUTTON)) {
						AEAudioPlay(clickSound, bgm.uiGroup, 0.6f, 0.6f, 0);
						for (size_t x = 0; x < ShopFunctions::GetInstance()->getPurchaseMultiplier(); x++)
						{
							ShopFunctions::GetInstance()->buyShopUpgrade(currentStat);
						}
						
						std::cout << "Increased " << shopButtons[i].label << " to " << ShopFunctions::GetInstance()->getStatBonus(currentStat) << std::endl;
					}
				}
			}
		}
	//}
}

void ShopState::Draw()
{
	/*if (isGachaActive)
	{
		AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
		AEGfxStart();
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		DrawGachaOverlay(gStateAnim, gachaFont);
	}
	else
	{*/
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
		// Draw Coins
		// ----------------
		AEVec2 coinLabelPos = DefaultToWorld(1500.f, 50.f);
		DrawAEText(Font, "COINS:", coinLabelPos, scale, CreateColor(255, 215, 0, 255), TEXT_MIDDLE);

		char coinAmount[64];
		snprintf(coinAmount, sizeof(coinAmount), "%d", ShopFunctions::GetInstance()->getMoney());
		AEVec2 coinAmtPos = DefaultToWorld(1500.f, 95.f);
		DrawAEText(Font, coinAmount, coinAmtPos, scale, CreateColor(255, 215, 0, 255), TEXT_MIDDLE);

		// ----------------
		// Draw Buttons
		// ----------------

		for (int i = 0; i < SHOP_BTN_COUNT; ++i)
		{
			AEVec2 worldPos = DefaultToWorld(shopButtons[i].pos.x, shopButtons[i].pos.y);
			AEVec2 worldSize = { shopButtons[i].size.x * scale, shopButtons[i].size.y * scale };
			bool hover = IsCursorOverWorld(worldPos, worldSize.x, worldSize.y, true);

			// Main Button Rect
			GetTransformMtx(mtx, worldPos, 0.0f, worldSize);
			AEGfxSetTransform(mtx.m);
			AEGfxSetColorToMultiply(hover ? 0.9f : 0.75f, hover ? 0.9f : 0.75f, hover ? 0.9f : 0.75f, 1.0f);
			AEGfxMeshDraw(squareMesh, AE_GFX_MDM_TRIANGLES);
			DrawAEText(Font, shopButtons[i].label, worldPos, scale, CreateColor(10, 10, 10, 255), TEXT_MIDDLE);

			// Side Buttons Rendering
			float sideBtnSize = 60.0f * scale;
			float sideOffset = (shopButtons[i].size.x / 2.0f) * scale - sideBtnSize / 2;
			AEVec2 minusPos = { worldPos.x - sideOffset, worldPos.y };
			AEVec2 plusPos = { worldPos.x + sideOffset, worldPos.y };
			if (shopButtons[i].hasSideButtons) {
				SideHover currentSideHover = NONE;
				if (IsCursorOverWorld(minusPos, sideBtnSize, shopButtons[i].size.y, true)) {
					currentSideHover = MINUS;
				}
				else if (IsCursorOverWorld(plusPos, sideBtnSize, shopButtons[i].size.y, true)) {
					currentSideHover = PLUS;
				}
				DrawSideButtons(shopButtons[i], currentSideHover);
				
				// Draw Level and Cost underneath the button for stat upgrades (0-4)
				if (i >= 0 && i <= 4) {
					STAT_TYPE currentStat =
						(i == 0) ? STAT_TYPE::ATT :
						(i == 1) ? STAT_TYPE::ATT_SPD :
						(i == 2) ? STAT_TYPE::MOVE_SPD :
						(i == 3) ? STAT_TYPE::MAX_HP : STAT_TYPE::DEF;

					int lvl = ShopFunctions::GetInstance()->getUpgradeLevel(currentStat);
					int cost = ShopFunctions::GetInstance()->calculatePrice(currentStat);

					char infoText[64];
					snprintf(infoText, sizeof(infoText), "Lv.%d | Cost: %d", lvl, cost);
					
					// position text right below the button
					AEVec2 textPos = { worldPos.x, worldPos.y - (shopButtons[i].size.y / 2.0f + 25.0f) * scale };
					DrawAEText(Font, infoText, textPos, scale * 0.8f, CreateColor(220, 220, 220, 255), TEXT_MIDDLE);
				}
			}
		}
	//}
}

void ShopState::ExitState()
{
	std::cout << "Exit shop state\n";
	//bgm.StopGacha(0.2f);
	//gStateAnim.Reset();
}

void ShopState::UnloadState() {
	//unload fonts
	if (Font >= 0) AEGfxDestroyFont(Font);
	if (BigFont >= 0) AEGfxDestroyFont(BigFont);

	// Unload button audio
	AEAudioUnloadAudio(hoverSound);
	AEAudioUnloadAudio(clickSound);

	//gachaFont = -1;
}
