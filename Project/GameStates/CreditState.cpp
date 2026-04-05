#include "CreditState.h"
#include "GameStateManager.h"
#include "../Helpers/Vec2Utils.h"
#include "../Helpers/CoordUtils.h"
#include "../Helpers/CollisionUtils.h"
#include "../Helpers/MatrixUtils.h"
#include "../Rendering/RenderingManager.h"
#include "../Helpers/RenderUtils.h"
#include "../Helpers/ColorUtils.h"
#include "../UI/UIConfig.h"
#include "../Music.h"
#include <iostream>

namespace {
	AEGfxVertexList* squareMesh = nullptr;
	AEGfxTexture* texClosed = nullptr;
	AEGfxTexture* texOpen = nullptr;
	s8 Font = -1;

	constexpr float DEFAULT_W = 1600.0f;
	constexpr float DEFAULT_H = 900.0f;

	float winW, winH, scale;

	AEAudioGroup audioGroup;
	AEAudio bgMusic;

    enum SCROLL_STATE { STATE_WAITING, STATE_UNROLLING, STATE_SCROLLING };
    SCROLL_STATE state = STATE_WAITING;

	float scrollBodyHeight = 0.0f;
	const float SCROLL_FULL_HEIGHT = 680.0f;
	const float SCROLL_UNROLL_SPEED = 10.0f;
	const float SCROLL_W = 580.0f;
	const float SPRITE_H = 75.0f;
	const float SPRITE_CAP_TOP = 14.0f;
	const float SPRITE_CAP_BOT = 14.0f;
	const float TOP_PADDING = 80.0f;
	const float BOTTOM_PADDING = 80.0f;
	const float CAP_H = SCROLL_FULL_HEIGHT * (SPRITE_CAP_TOP / SPRITE_H);
	float yPos_credits = 0.0f;
	const float LINE_H = 58.0f;
	const float SCROLL_SPEED = 1.0f;
	float flashTimer = 0.0f;

	AEGfxTexture* logoTex = nullptr;
	const float LOGO_DISPLAY_W = 320.0f;
	const float LOGO_DISPLAY_H = 110.0f;
	const float LOGO_GAP = 180.0f;

	AEVec2 DefaultToWorld(float x, float y)
	{
		return {
			(x - DEFAULT_W * 0.5f) * scale,
			(DEFAULT_H * 0.5f - y) * scale
		};
	}

	const char* credits[] = {
	   "Team STD::Null",
	   "Team Members:",
	   "Xavier Lim",
	   "TEAM LEAD and PROGRAMMER",
	   "Edna Sim",
	   "TECH LEAD and PROGRAMMER",
	   "Hong Teck",
	   "GAMEPLAY LEAD and PROGRAMMER",
	   "Joon Hin",
	   "UI LEAD and PROGRAMMER",
	   " ",
	   "Faculty and Advisors",
	   "Instructors",
	   "DR Soroor Malekmohammadi Faradounbeh",
	   "MR Tan Chee Wei, Tommy",
	   "MR Wong Han Feng, Gerald",
	   " ",
	   "SPECIAL THANKS TO",
	   "NParks falcon chicks cam",
	   "Cookie (Xavier's dog)",
	   "Pepper (Edna's pet bird)",
	   " ",
	   "Created at",
	   "DigiPen Institute of Technology Singapore",
	   "PRESIDENT",
	   "Claude COMAIR",
	   "EXECUTIVES",
	   "Mandy WONG    Johnny DEEK",
	   "TAN Chek Ming    Prasanna Kumar GHALI",
	   "Claude COMAIR    CHU Jason Yeu Tat    Michael GATS",
	   " ",
	   "Copyrights for software, tools and libraries",
	   "(C) MetaDigger",
	   "(C) Kenney Assets",
	   " ",
	   "All content (c) 2026 DigiPen Institute of Technology Singapore. All Rights Reserved",
	   nullptr
	};

	bool IsHeader(const char* line)
	{
		if (!line || line[0] == ' ' || line[0] == '\0') return false;
		for (int i = 0; line[i] != '\0'; ++i)
			if (line[i] >= 'a' && line[i] <= 'z') return false;
		return true;
	}

	void DrawScroll(float cx, float cy, float bodyH)
	{
		if (bodyH <= 0.0f) return;
		AEVec2 pos = { cx, cy };
		AEVec2 size = { SCROLL_W * scale, bodyH };
		DrawTintedMesh(GetTransformMtx(pos, 0.0f, size),
			squareMesh, texOpen, { 255, 255, 255, 255 }, 255);
	}

	void DrawScrollClosed(float cx, float cy)
	{
		AEVec2 pos = { cx, cy };
		AEVec2 size = { SCROLL_W * scale, (SCROLL_W * 0.5f) * scale };
		DrawTintedMesh(GetTransformMtx(pos, 0.0f, size),
			squareMesh, texClosed, { 255, 255, 255, 255 }, 255);
	}
}

void CreditState::LoadState()
{
	squareMesh = RenderingManager::GetInstance()->GetMesh(MESH_SQUARE);
	Font = AEGfxCreateFont(PRIMARY_FONT_PATH, 28);
	texClosed = RenderingManager::GetInstance()->LoadTexture("Assets/sprites/credits/scroll_closed.png");
	texOpen = RenderingManager::GetInstance()->LoadTexture("Assets/sprites/credits/scroll_open.png");
	logoTex = RenderingManager::GetInstance()->LoadTexture("Assets/digipen.png");

	audioGroup = AEAudioCreateGroup();

	bgMusic = AEAudioLoadSound("Assets/Audio/PROSPECTUS - Corporate MSCCRP1_50.wav");
}

void CreditState::InitState()
{
	winW = static_cast<float>(AEGfxGetWinMaxX());
	winH = static_cast<float>(AEGfxGetWinMaxY());
	scale = (winW * 2.0f / DEFAULT_W) < (winH * 2.0f / DEFAULT_H)
		? (winW * 2.0f / DEFAULT_W) : (winH * 2.0f / DEFAULT_H);

	state = STATE_WAITING;
	scrollBodyHeight = 0.0f;
	flashTimer = 0.0f;
	yPos_credits = 0.0f;

	AEAudioPlay(bgMusic, audioGroup, 1.0f, 1.0f, -1);
}

void CreditState::Update(double dt)
{
	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		GameStateManager::GetInstance()->SetNextGameState("MainMenuState", true, true);
		return;
	}

	if (state == STATE_WAITING) {
		flashTimer += static_cast<float>(dt);
		if (AEInputCheckTriggered(AEVK_LBUTTON) ||
			AEInputCheckTriggered(AEVK_SPACE) ||
			AEInputCheckTriggered(AEVK_RETURN)) {
			bgm.PlayUIClick();
			state = STATE_UNROLLING;
			scrollBodyHeight = 0.0f;
		}
		return;
	}

	float fullH = SCROLL_FULL_HEIGHT * scale;

	float clipWorldBottom = (fullH * 0.5f
		- SCROLL_FULL_HEIGHT * (SPRITE_CAP_BOT / SPRITE_H) * scale)
		- BOTTOM_PADDING * scale;

	if (state == STATE_UNROLLING) {
		scrollBodyHeight += SCROLL_UNROLL_SPEED * scale;
		if (scrollBodyHeight >= fullH) {
			scrollBodyHeight = fullH;
			state = STATE_SCROLLING;
			// Start logo just below the bottom clip so it scrolls up into view
			yPos_credits = -clipWorldBottom - (LOGO_DISPLAY_H * 0.5f) * scale;
		}
		return;
	}

	if (state == STATE_SCROLLING) {
		yPos_credits += SCROLL_SPEED * scale;

		int totalLines = 0;
		for (int i = 0; credits[i] != nullptr; ++i) totalLines++;

		// text disappears here
		float clipWorldTop = (-fullH * 0.5f
			+ SCROLL_FULL_HEIGHT * (SPRITE_CAP_TOP / SPRITE_H) * scale)
			+ TOP_PADDING * scale;

		// Reset when the last text line has fully scrolled past the top clip
		if (yPos_credits - (LOGO_GAP * scale) - (totalLines * LINE_H * scale) > -clipWorldTop) {
			yPos_credits = -clipWorldBottom - (LOGO_DISPLAY_H * 0.5f) * scale;
		}
	}
}

void CreditState::Draw()
{
	AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

	float cx = 0.0f;
	float cy = 0.0f;

	if (state == STATE_WAITING) {
		DrawScrollClosed(cx, cy);
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		if ((int)(flashTimer / 0.6f) % 2 == 0) {
			AEVec2 promptPos = { cx, cy - SCROLL_W * 0.3f * scale };
			DrawAEText(Font, "Touch to reveal", promptPos, scale,
				CreateColor(200, 170, 80, 255), TEXT_MIDDLE);
		}
		AEVec2 hintPos = DefaultToWorld(80.0f, DEFAULT_H - 40.0f);
		DrawAEText(Font, "[ESC] Back", hintPos, scale * 0.75f,
			CreateColor(140, 140, 140, 255), TEXT_MIDDLE);
		return;
	}

	float fullH = SCROLL_FULL_HEIGHT * scale;
	DrawScroll(cx, cy, (state == STATE_UNROLLING) ? scrollBodyHeight : fullH);

	if (state == STATE_UNROLLING) {
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		AEVec2 hintPos = DefaultToWorld(80.0f, DEFAULT_H - 40.0f);
		DrawAEText(Font, "[ESC] Back", hintPos, scale * 0.75f,
			CreateColor(140, 140, 140, 255), TEXT_MIDDLE);
		return;
	}

	float clipWorldTop = cy - fullH * 0.5f
		+ SCROLL_FULL_HEIGHT * (SPRITE_CAP_TOP / SPRITE_H) * scale
		+ TOP_PADDING * scale;
	float clipWorldBottom = cy + fullH * 0.5f
		- SCROLL_FULL_HEIGHT * (SPRITE_CAP_BOT / SPRITE_H) * scale
		- BOTTOM_PADDING * scale;

	// Draw DigiPen logo
	float logoY = yPos_credits;
	if (logoY > clipWorldTop && logoY < clipWorldBottom) {
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		AEVec2 logoPos = { cx, logoY };
		AEVec2 logoSize = { LOGO_DISPLAY_W * scale, LOGO_DISPLAY_H * scale };
		DrawTintedMesh(GetTransformMtx(logoPos, 0.0f, logoSize),
			squareMesh, logoTex, { 255, 255, 255, 255 }, 255);
	}

	// Draw text lines
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	int totalLines = 0;
	for (int i = 0; credits[i] != nullptr; ++i) totalLines++;

	for (int i = 0; i < totalLines; ++i) {
		float y = yPos_credits - (LOGO_GAP * scale) - (i * LINE_H * scale);

		if (y < clipWorldTop || y > clipWorldBottom) continue;

		// copyright notice line modified as its too fat
		if (i == totalLines - 1) {
			DrawAEText(Font, credits[i], { cx, y }, scale * 0.53f,
				CreateColor(50, 35, 15, 255), TEXT_MIDDLE);
		}
		else if (IsHeader(credits[i])) {
			DrawAEText(Font, credits[i], { cx, y }, scale * 0.9f,
				CreateColor(160, 110, 10, 255), TEXT_MIDDLE);
		}
		else {
			DrawAEText(Font, credits[i], { cx, y }, scale * 0.72f,
				CreateColor(40, 25, 10, 255), TEXT_MIDDLE);
		}
	}

	AEVec2 hintPos = DefaultToWorld(80.0f, DEFAULT_H - 40.0f);
	DrawAEText(Font, "[ESC] Back", hintPos, scale * 0.75f,
		CreateColor(140, 140, 140, 255), TEXT_MIDDLE);
}

void CreditState::ExitState()
{
	AEAudioStopGroup(audioGroup);
}

void CreditState::UnloadState()
{
	if (Font >= 0) AEGfxDestroyFont(Font);
	AEAudioUnloadAudio(bgMusic);
	AEAudioUnloadAudioGroup(audioGroup);
}