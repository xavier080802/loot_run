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
#include "../gacha.h"
#include "../Music.h"


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
		{{ 500.f, 300.f }, { 450.f, 144.f }, "Damage"},
		{{ 500.f, 500.f }, { 450.f, 144.f }, "Attack Speed"},
		{{ 500.f, 700.f }, { 450.f, 144.f }, "Move Speed"},
		{{ 1100.f, 300.f }, { 450.f, 144.f }, "Health"},
		{{ 1100.f, 500.f }, { 450.f, 144.f }, "Dodge"},
		{{ 1100.f, 700.f }, { 450.f, 144.f }, "Enter Gacha"}
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


	AEAudioGroup buttonGroup;
	AEAudio hoverSound;

	AEAudio clickSound;

	// Track previous hover state
	bool btnHoverStates[SHOP_BTN_COUNT] = { false };
	static GachaAnimation gStateAnim;
	static s8 gachaFont = -1;
	bool isGachaActive = false;
}
void ShopState::LoadState()
{
	squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);

	buttonGroup = AEAudioCreateGroup();
	hoverSound = AEAudioLoadSound("Assets/Audio/MOUSETRAP_GEN-HDF-17767.wav");
	clickSound = AEAudioLoadSound("Assets/Audio/MOUSETRAP_GEN-HDF-17766.wav");
	gachaFont = AEGfxCreateFont("Assets/Exo2/Exo2-SemiBoldItalic.ttf", 32);
	EnsureOverlayMesh();
}

void ShopState::InitState()
{
	std::cout << "Shop state enter\n";
	AEGfxFontSystemStart();
	Font = AEGfxCreateFont("Assets/Exo2-Regular.ttf", 38);
	BigFont = AEGfxCreateFont("Assets/Exo2-Regular.ttf", 75);
	winW = static_cast<float>(AEGfxGetWinMaxX());
	winH = static_cast<float>(AEGfxGetWinMaxY());
	scale = (winW * 2 / DEFAULT_W) < (winH * 2 / DEFAULT_H) ? (winW * 2 / DEFAULT_W) : (winH * 2 / DEFAULT_H); //scale of window compared to default

	// Reset hover tracking when entering state
	for (int i = 0; i < SHOP_BTN_COUNT; ++i) btnHoverStates[i] = false;
	isGachaActive = false;
}

void ShopState::ExitState()
{
	std::cout << "Exit shop state\n";
	bgm.StopGacha(0.2f);
	gStateAnim.Reset();
}

void ShopState::UnloadState()
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
	gachaFont = -1;
}

void ShopState::Update(double dt)
{
	if (isGachaActive)
	{
		bool openPressed = AEInputCheckTriggered(AEVK_O) || AEInputCheckTriggered(0x4F);
		bool skipPressed = AEInputCheckTriggered(AEVK_SPACE);
		bool pull10 = AEInputCheckTriggered(AEVK_R) || AEInputCheckTriggered(0x52);
		bool pull100 = AEInputCheckTriggered(AEVK_T) || AEInputCheckTriggered(0x54);
		bool exitPressed = AEInputCheckTriggered(AEVK_ESCAPE);

		if (exitPressed) {
			isGachaActive = false;
			bgm.StopGacha(0.2f);
			return;
		}

		if (gStateAnim.phase == GachaPhase::Done) {
			if (pull10) BeginGachaOverlay(gStateAnim, 10, 0.1f, 0.8f, 0.3f);
			else if (pull100) BeginGachaOverlay(gStateAnim, 100, 0.1f, 1.2f, 0.2f);
		}

		UpdateGachaOverlay(gStateAnim, static_cast<float>(dt), skipPressed, openPressed);
		if (gStateAnim.isFinished) gStateAnim.phase = GachaPhase::Done;
	}
	else
	{
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
			bool buttonHover = IsCursorOver(worldPos, worldSize.x, worldSize.y);
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
					case 0: // Damage

						break;
					case 1: // Attack Speed

						break;
					case 2: // Move Speed

						break;
					case 3: // Health

						break;
					case 4: // Dodge

						break;
					case 5: // Gacha Trigger
						isGachaActive = true;
						bgm.StopGameplayBGM();
						bgm.PlayGacha();
						BeginGachaOverlay(gStateAnim, 10, 0.6f, 1.2f, 0.3f);
						break;
					}
				}
			}
		}
	}
}

void ShopState::Draw()
{
	if (isGachaActive)
	{
		AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
		AEGfxStart();
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		DrawGachaOverlay(gStateAnim, gachaFont);
	}
	else
	{
		AEGfxSetBackgroundColor(0.2f, 0.2f, 0.2f);
		AEGfxStart();
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

			bool hover = IsCursorOver(worldPos, worldSize.x, worldSize.y);

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
				Font, shopButtons[i].label, worldPos, scale,
				CreateColor(10, 10, 10, 255),
				TEXT_MIDDLE
			);
		}
	}
}