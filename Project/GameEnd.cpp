#include "GameEnd.h"
#include "GameStateManager.h"
#include "Helpers/Vec2Utils.h"
#include "Helpers/CollisionUtils.h"
#include "Helpers/MatrixUtils.h"
#include "Helpers/RenderUtils.h"
#include "Helpers/ColorUtils.h"
#include "RenderingManager.h"
#include "ShopFunctions.h"
#include "UIConfig.h"

// =====================================================================
// Internal state
// =====================================================================
namespace
{
	constexpr float DEFAULT_W = 1600.f;
	constexpr float DEFAULT_H = 900.f;

	// Panel dimensions (default-space)
	constexpr float POP_CX = DEFAULT_W / 2.f;
	constexpr float POP_CY = DEFAULT_H / 2.f;
	constexpr float POP_W  = 700.f;
	constexpr float POP_H  = 580.f;

	constexpr float POP_TOP = POP_CY - POP_H / 2.f;

	// Row Y positions inside the panel (default-space, top-down)
	constexpr float ROW_TITLE     = POP_TOP + 80.f;
	constexpr float ROW_COINS     = POP_TOP + 205.f;
	constexpr float ROW_KILLS     = POP_TOP + 275.f;
	constexpr float ROW_TIME      = POP_TOP + 355.f;   // endless only
	constexpr float ROW_HIGHSCORE = POP_TOP + 420.f;   // endless only
	constexpr float ROW_HINT	  = POP_TOP + 650.f;

	// Button Y: shift down further when endless rows are shown
	constexpr float ROW_BTNS_STD  = POP_TOP + 460.f;
	constexpr float ROW_BTNS_END  = POP_TOP + 510.f;

	// Button dimensions and layout
	constexpr float BTN_W    = 240.f;
	constexpr float BTN_H    = 70.f;
	constexpr float BTN_GAP  = 50.f;

	constexpr float BTN_LEFT  = POP_CX - BTN_W / 2.f - BTN_GAP / 2.f;
	constexpr float BTN_RIGHT = POP_CX + BTN_W / 2.f + BTN_GAP / 2.f;

	// Runtime state
	bool  overlayOpen = false;
	bool  isWin       = false;
	bool  isEndless   = false;
	float runTime     = 0.f;
	int   coinsGained = 0;
	int   killCount   = 0;

	// Hints
	const char* hintTexts[] = {
		"You can use the coins you earn to buy upgrades in the shop!",
		"Collect heath pickups to survive longer!",
		"It is ok to retreat from strong enemies!",
		"Keep an eye on your ammo and use your ranged attacks wisely!",
		"Your dodge has invincibility frames! Use it to avoid damage and reposition yourself!",
		"Try attacking and dodging at the same time!",
		"Pets can be helpful companions in battle. Press R to use your pet's ability!",
		"You can press and hold Left Click to auto attack!",
		"Try kiting enemies using ranged weapons!"
		//"Skill issue. Get Gud. L + Ratio"
	};
	constexpr int HINT_COUNT = sizeof(hintTexts) / sizeof(hintTexts[0]);
	int hintIndex = 0;
	bool hintsSeeded = false;

	// Hint timer (seconds)
	constexpr float HINT_INTERVAL = 8.f;
	float hintTimer = 0.f;

	// Hover tracking: [0] = Retry  [1] = Main Menu
	bool hoverStates[2] = { false };

	// Fonts – created lazily in EnsureFonts(), destroyed in Hide()
	s8 fontTitle = -1;
	s8 fontBody  = -1;

	// ---------------------------------------------------------------
	// Helpers
	// ---------------------------------------------------------------
	float GetScale()
	{
		float winW = static_cast<float>(AEGfxGetWinMaxX()) * 2.f;
		float winH = static_cast<float>(AEGfxGetWinMaxY()) * 2.f;
		return (winW / DEFAULT_W) < (winH / DEFAULT_H)
			   ? (winW / DEFAULT_W) : (winH / DEFAULT_H);
	}

	AEVec2 D2W(float x, float y, float scale)
	{
		return {
			(x - DEFAULT_W / 2.f) * scale,
			(DEFAULT_H / 2.f  - y) * scale
		};
	}

	AEGfxVertexList* GetMesh()
	{
		return RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);
	}

	void DrawRect(float cx, float cy, float w, float h, float scale,
				  float r, float g, float b, float a = 1.f)
	{
		AEVec2  pos  = D2W(cx, cy, scale);
		AEVec2  size = { w * scale, h * scale };
		AEMtx33 mtx;
		GetTransformMtx(mtx, pos, 0.f, size);
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxTextureSet(nullptr, 0, 0);
		AEGfxSetTransform(mtx.m);
		AEGfxSetColorToMultiply(r, g, b, a);
		AEGfxMeshDraw(GetMesh(), AE_GFX_MDM_TRIANGLES);
	}

	bool Hovered(float cx, float cy, float w, float h, float scale)
	{
		AEVec2 pos  = D2W(cx, cy, scale);
		AEVec2 size = { w * scale, h * scale };
		return IsCursorOverWorld(pos, size.x, size.y, true);
	}

	void EnsureFonts()
	{
		if (fontTitle < 0)
			fontTitle = AEGfxCreateFont(PRIMARY_FONT_PATH, 100);
		if (fontBody < 0)
			fontBody  = AEGfxCreateFont(PRIMARY_FONT_PATH, 46);
	}

	void DestroyFonts()
	{
		if (fontTitle >= 0) { AEGfxDestroyFont(fontTitle); fontTitle = -1; }
		if (fontBody  >= 0) { AEGfxDestroyFont(fontBody);  fontBody  = -1; }
	}

} // anonymous namespace

// =====================================================================
// Public interface
// =====================================================================
namespace GameEnd
{
	void Show(bool won, bool endless, float time, int coins, int kills)
	{
		bool wasOpen = overlayOpen;
		overlayOpen = true;
		isWin       = won;
		isEndless   = endless;
		runTime     = time;
		coinsGained = coins;
		killCount   = kills;
		hoverStates[0] = hoverStates[1] = false;
		// Seed random once and pick a hint
		if (!hintsSeeded)
		{
			std::srand(static_cast<unsigned>(std::time(nullptr)));
			hintsSeeded = true;
		}
		// Only choose a new hint when the overlay is opened (transition from closed -> open)
		if (!wasOpen)
		{
			hintIndex = std::rand() % HINT_COUNT;
			hintTimer = 0.f;
		}

		EnsureFonts();
	}

	void Hide()
	{
		overlayOpen = false;
		DestroyFonts();
	}

	bool IsOpen() { return overlayOpen; }

	// ------------------------------------------------------------------
	// Zeroes dt so every system that receives it (timers, physics, AI,
	// animations) naturally freezes. Then processes button clicks.
	// ------------------------------------------------------------------
	void Update(double& dt)
	{
		if (!overlayOpen) return;

		// Freeze the game world by killing the time step
		dt = 0.0;

		float scale   = GetScale();
		bool  clicked = AEInputCheckTriggered(AEVK_LBUTTON);
		float btnY    = isEndless ? ROW_BTNS_END : ROW_BTNS_STD;

		hoverStates[0] = Hovered(BTN_LEFT,  btnY, BTN_W, BTN_H, scale);
		hoverStates[1] = Hovered(BTN_RIGHT, btnY, BTN_W, BTN_H, scale);

		if (clicked)
		{
			if (hoverStates[0]) // Retry Level Select
			{
				Hide();
				GameStateManager::GetInstance()
					->SetNextGameState("LevelSelectState", true, true);
			}
			else if (hoverStates[1]) // Main Menu
			{
				Hide();
				GameStateManager::GetInstance()
					->SetNextGameState("MainMenuState", true, true);
			}
		}
	}

	// ------------------------------------------------------------------
	void Draw()
	{
		if (!overlayOpen) return;

		EnsureFonts();

		float scale = GetScale();

		// Advance hint timer using AlphaEngine frame time and rotate hint every HINT_INTERVAL seconds
		hintTimer += static_cast<float>(AEFrameRateControllerGetFrameTime());
		if (hintTimer >= HINT_INTERVAL) {
			hintTimer = 0.f;
			hintIndex = (hintIndex + 1) % HINT_COUNT;
		}

		AEGfxSetBlendMode(AE_GFX_BM_BLEND);

		// ---- Semi-transparent dim over the frozen game world ----
		DrawRect(DEFAULT_W / 2.f, DEFAULT_H / 2.f,
				 DEFAULT_W, DEFAULT_H, scale,
				 0.f, 0.f, 0.f, 0.62f);

		// ---- Panel background ----
		DrawRect(POP_CX, POP_CY, POP_W, POP_H, scale, 0.15f, 0.15f, 0.15f);

		// ---- Coloured title bar (gold for win, red for defeat) ----
		if (isWin)
			DrawRect(POP_CX, ROW_TITLE, POP_W, 115.f, scale, 0.50f, 0.42f, 0.04f);
		else
			DrawRect(POP_CX, ROW_TITLE, POP_W, 115.f, scale, 0.52f, 0.07f, 0.07f);

		// ---- Title text ----
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		const char* titleStr = isWin ? "YOU WIN!" : "YOU DIED!";
		Color        titleCol = isWin
								? CreateColor(255, 220,  50, 255)
								: CreateColor(255,  60,  60, 255);
		DrawAEText(fontTitle, titleStr,
				   D2W(POP_CX, ROW_TITLE, scale),
				   scale * 0.85f,
				   titleCol, TEXT_MIDDLE);

		// ---- Coins gained ----
		char coinBuf[64];
		snprintf(coinBuf, sizeof(coinBuf), "Coins Gained:   %d", coinsGained);
		DrawAEText(fontBody, coinBuf,
				   D2W(POP_CX, ROW_COINS, scale),
				   scale * 0.72f,
				   CreateColor(255, 215, 0, 255), TEXT_MIDDLE);

		// ---- Kill count (always shown) ----
		char killBuf[64];
		snprintf(killBuf, sizeof(killBuf), "Enemies Killed:  %d", killCount);
		DrawAEText(fontBody, killBuf,
				   D2W(POP_CX, ROW_KILLS, scale),
				   scale * 0.72f,
				   CreateColor(220, 100, 100, 255), TEXT_MIDDLE);

		// ---- Endless-only rows ----
		if (isEndless)
		{
			int   totalSecs = (int)runTime;
			int   m = totalSecs / 60;
			int   s = totalSecs % 60;
			float highScore = ShopFunctions::GetInstance()->getEndlessHighScore();
			int   hm = (int)highScore / 60;
			int   hs = (int)highScore % 60;

			char timeBuf[64], hsBuf[64];
			snprintf(timeBuf, sizeof(timeBuf), "Run Time:    %02d:%02d", m,  s);
			snprintf(hsBuf,   sizeof(hsBuf),   "Best Time:   %02d:%02d", hm, hs);

			DrawAEText(fontBody, timeBuf,
					   D2W(POP_CX, ROW_TIME, scale),
					   scale * 0.72f,
					   CreateColor(200, 200, 200, 255), TEXT_MIDDLE);

			DrawAEText(fontBody, hsBuf,
					   D2W(POP_CX, ROW_HIGHSCORE, scale),
					   scale * 0.72f,
					   CreateColor(255, 215, 0, 255), TEXT_MIDDLE);
		}

		// ---- Hints ----
		const char* hintText = hintTexts[hintIndex % HINT_COUNT];
		char HintBuf[128];
		snprintf(HintBuf, sizeof(HintBuf), "Hint:  %s", hintText);
		DrawAEText(fontBody, HintBuf,
			D2W(POP_CX, ROW_HINT, scale),
			scale * 0.72f,
			CreateColor(220, 220, 170, 255), TEXT_MIDDLE);

		// ---- Buttons ----
		float btnY = isEndless ? ROW_BTNS_END : ROW_BTNS_STD;

		// Retry / Level Select  (green)
		{
			float t = hoverStates[0] ? 0.95f : 0.72f;
			DrawRect(BTN_LEFT, btnY, BTN_W, BTN_H, scale,
					 t * 0.28f, t * 0.72f, t * 0.28f);
			AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
			DrawAEText(fontBody, "Retry",
					   D2W(BTN_LEFT, btnY, scale),
					   scale * 0.68f,
					   CreateColor(10, 10, 10, 255), TEXT_MIDDLE);
		}

		// Main Menu  (red)
		{
			float t = hoverStates[1] ? 0.95f : 0.72f;
			DrawRect(BTN_RIGHT, btnY, BTN_W, BTN_H, scale,
					 t * 0.72f, t * 0.28f, t * 0.28f);
			AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
			DrawAEText(fontBody, "Main Menu",
					   D2W(BTN_RIGHT, btnY, scale),
					   scale * 0.68f,
					   CreateColor(10, 10, 10, 255), TEXT_MIDDLE);
		}

		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	}

} // namespace GameEnd
