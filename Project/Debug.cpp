// Handles all debug and overlay rendering for GameState.

#include "Debug.h"
#include "../Project/Helpers/RenderUtils.h"
#include "../Project/Helpers/MatrixUtils.h"
#include "../Project/RenderingManager.h"
#include "../Project/GameObjects/GameObjectManager.h"
#include "../Project/GameObjects/GameObject.h"
#include "../Project/Actor/Enemy.h"
#include "../Project/TileMap.h"
#include <sstream>
#include <cmath>
#include <iostream>
#include "../Project/GameObjects/LootChest.h"


// Draws a semi-transparent dark panel background at the given top-left position
void DrawPanel(const DebugContext& ctx, float screenX, float screenY, float w, float h)
{
	// Calculate center position for the mesh transform
	AEVec2  pos = { screenX + w * 0.5f, screenY - h * 0.5f };
	AEMtx33 mtx;
	GetTransformMtx(mtx, pos, 0, { w, h });

	// Setup graphics state for untextured colored geometry
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxTextureSet(nullptr, 0, 0);
	AEGfxSetTransform(mtx.m);

	// Apply dark tint with transparency for overlay background
	AEGfxSetColorToMultiply(0.05f, 0.05f, 0.05f, 0.88f);
	AEGfxMeshDraw(ctx.squareMesh, AE_GFX_MDM_TRIANGLES);
}

// Keybind overlay – shown when player presses K
void DrawKeybindOverlay(const DebugContext& ctx)
{
	// MASTER TOGGLE: Return immediately if debug is globally disabled
	if (!ctx.masterDebugEnabled || ctx.font < 0) return;

	// Calculate panel positioning relative to camera and window bounds
	AEVec2 camPos;
	AEGfxGetCamPosition(&camPos.x, &camPos.y);

	float winW = (float)(AEGfxGetWinMaxX() - AEGfxGetWinMinX());
	float winH = (float)(AEGfxGetWinMaxY() - AEGfxGetWinMinY());

	float panelW = 420.f;
	float panelH = winH - 20.f;
	float panelX = camPos.x + (winW * 0.5f) - panelW - 10.f;
	float panelY = camPos.y + (winH * 0.5f) - 10.f;

	// Render the background panel
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	DrawPanel(ctx, panelX, panelY, panelW, panelH);
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

	// Text formatting constants
	s8    fnt = RenderingManager::GetInstance()->GetFont();
	float pad = 12.f;
	float keyW = 120.f;
	float x = panelX + pad;
	float y = panelY - pad;
	float lineH = 30.f;
	float sz = 0.34f;

	// Draw menu title
	DrawAEText(fnt, "=== KEYBINDS ===", { x, y }, sz, CreateColor(255, 200, 60, 255), TEXT_MIDDLE_LEFT);
	y -= lineH + 6.f;

	// Helper for rendering category headers
	auto Header = [&](const char* txt) {
		y -= 7.f; // secGapTop
		DrawAEText(fnt, txt, { x, y }, sz, CreateColor(100, 180, 255, 255), TEXT_MIDDLE_LEFT);
		y -= lineH + 1.f; // secGapBot
		};

	// Helper for rendering key/description pairs
	auto KV = [&](const char* key, const char* desc) {
		DrawAEText(fnt, key, { x,        y }, sz, CreateColor(255, 255, 255, 255), TEXT_MIDDLE_LEFT);
		DrawAEText(fnt, desc, { x + keyW, y }, sz, CreateColor(176, 176, 176, 255), TEXT_MIDDLE_LEFT);
		y -= lineH;
		};

	// --- Control Sections ---
	Header("--- Movement ---");
	KV("[W/A/S/D]", "Move");
	KV("[SPACE]", "Dodge roll");

	Header("--- Combat ---");
	KV("[LMB]", "Attack");
	KV("[Z]", "Weapon slot 1");
	KV("[X]", "Weapon slot 2");
	KV("[Q]", "Equip bow");
	KV("[RMB]", "Swap melee weapons");
	KV("[G]", "Drop held weapon");

	Header("--- Interaction ---");
	KV("[E]", "Swap item (slot full)");
	KV("[C]", "Sell item");

	Header("--- UI ---");
	KV("[B]", "Stats & equipment");
	KV("[K]", "Keybind overlay");
	KV("[TAB]", "Debug overlay");
	KV("[M]", "Main menu");

	Header("--- Pets ---");
	KV("[R]", "Cast pet skill");

	Header("--- Dev / Testing ---");
	KV("[N]", "Spawn enemy");
	KV("[L]", "Move chest here");
	KV("[F7]", "Highlight chests");
}

// Debug overlay – shown when TAB is pressed in debug mode
void DrawDebugOverlay(const DebugContext& ctx)
{
	// MASTER TOGGLE: Return immediately if debug is globally disabled
	if (!ctx.masterDebugEnabled || ctx.font < 0 || !ctx.gPlayer) return;

	// Screen and Camera positioning
	AEVec2 camPos;
	AEGfxGetCamPosition(&camPos.x, &camPos.y);

	float winW = (float)(AEGfxGetWinMaxX() - AEGfxGetWinMinX());
	float winH = (float)(AEGfxGetWinMaxY() - AEGfxGetWinMinY());

	float panelW = 420.f;
	float panelH = winH - 20.f;

	// Offset panel if the keybind overlay is also open to prevent clipping
	float panelX = ctx.showKeybindOverlay
		? camPos.x + (winW * 0.5f) - 420.f - 10.f - panelW - 8.f
		: camPos.x + (winW * 0.5f) - panelW - 10.f;
	float panelY = camPos.y + (winH * 0.5f) - 10.f;

	// Render background
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	DrawPanel(ctx, panelX, panelY, panelW, panelH);
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);

	// Layout variables
	s8    fnt = RenderingManager::GetInstance()->GetFont();
	float pad = 12.f;
	float x = panelX + pad;
	float valOffset = 200.f;
	float y = panelY - pad;
	float lineH = 30.f;
	float sz = 0.34f;

	// Helper for key-value diagnostic pairs
	auto KV = [&](const char* key, const char* val, float vr = 0.4f, float vg = 1.f, float vb = 0.9f) {
		DrawAEText(fnt, key, { x,              y }, sz, CreateColor(220, 220, 220, 255), TEXT_MIDDLE_LEFT);
		DrawAEText(fnt, val, { x + valOffset, y }, sz, CreateColor((u8)(vr * 255), (u8)(vg * 255), (u8)(vb * 255), 255), TEXT_MIDDLE_LEFT);
		y -= lineH;
		};

	// Helper for colored section headers
	auto SecHeader = [&](const char* txt) {
		y -= 7.f;
		DrawAEText(fnt, txt, { x, y }, sz, CreateColor(100, 180, 255, 255), TEXT_MIDDLE_LEFT);
		y -= lineH + 1.f;
		};

	// Helper for binary toggle states [ON]/[off]
	auto Toggle = [&](const char* key, const char* label, bool state) {
		std::ostringstream lbl;
		lbl << key << "  " << label;
		DrawAEText(fnt, lbl.str().c_str(), { x, y }, sz, CreateColor(176, 176, 176, 255), TEXT_MIDDLE_LEFT);
		DrawAEText(fnt, state ? "[ON]" : "[off]", { x + 200.f, y }, sz,
			CreateColor(state ? 80.f : 102.f, state ? 238.f : 102.f, state ? 80.f : 102.f, 255.f), TEXT_MIDDLE_LEFT);
		y -= lineH;
		};

	// Helper for executable dev actions
	auto ActKV = [&](const char* key, const char* desc) {
		DrawAEText(fnt, key, { x,              y }, sz, CreateColor(204, 204, 204, 255), TEXT_MIDDLE_LEFT);
		DrawAEText(fnt, desc, { x + valOffset, y }, sz, CreateColor(204, 204, 204, 255), TEXT_MIDDLE_LEFT);
		y -= lineH;
		};

	// --- Debug Title ---
	DrawAEText(fnt, "=== DEBUG MODE ===", { x, y }, sz, CreateColor(255, 172, 0, 255), TEXT_MIDDLE_LEFT);
	y -= lineH + 6.f;

	// --- Live Player Stats ---
	std::ostringstream oss;
	AEVec2 pp = ctx.gPlayer->GetPos();
	oss << (int)pp.x << ", " << (int)pp.y;
	KV("Pos:", oss.str().c_str());

	oss.str(""); oss << (int)ctx.gPlayer->GetHP() << " / " << (int)ctx.gPlayer->GetMaxHP();
	bool lowHP = ctx.gPlayer->GetHP() < ctx.gPlayer->GetMaxHP() * 0.3f;
	KV("HP:", oss.str().c_str(), lowHP ? 1.f : 0.4f, lowHP ? 0.3f : 1.f, 0.4f);

	KV("Map:", ctx.inProceduralMap ? "Procedural" : "CSV");

	// --- Enemy Spawn Monitoring ---
	SecHeader("--- Enemy Counts ---");

	oss.str(""); oss << ctx.csvEnemyCount;
	KV("CSV spawned:", oss.str().c_str());

	oss.str(""); oss << ctx.procEnemyCount;
	KV("Proc spawned:", oss.str().c_str());

	oss.str(""); oss << ctx.totalEnemiesKilled << " / " << ctx.totalKillTarget;
	KV("Kills/Target:", oss.str().c_str(),
		ctx.totalEnemiesKilled >= ctx.totalKillTarget ? 0.2f : 0.4f,
		ctx.totalEnemiesKilled >= ctx.totalKillTarget ? 1.0f : 1.0f,
		0.4f);

	KV("Boss:", ctx.bossSpawned ? "SPAWNED" : "waiting",
		ctx.bossSpawned ? 1.f : 0.6f,
		ctx.bossSpawned ? 0.4f : 0.8f,
		0.4f);

	// --- Dev Toggles ---
	SecHeader("--- Toggles ---");
	Toggle("[F5]", "God mode", ctx.debugGodMode);
	Toggle("[F6]", "Freeze AI", ctx.debugFreezeEnemies);
	Toggle("[F7]", "Show chests", ctx.debugShowChests);

	// --- Dev Commands ---
	SecHeader("--- Actions ---");
	ActKV("[F1]", "Kill all enemies");
	ActKV("[F2]", "Force-spawn boss");
	ActKV("[F3]", "Teleport to spawn");
	ActKV("[F4]", "Refill HP");
	ActKV("[N]", "Spawn enemy");
	ActKV("[L]", "Spawn chest");
}

// Displays names and health above active enemy actors
void DrawEnemyStats(const DebugContext& ctx)
{
	if (!ctx.masterDebugEnabled || ctx.font < 0) return;

	auto& gos = GameObjectManager::GetInstance()->GetGameObjects();
	for (GameObject* go : gos) {
		if (!go || !go->IsEnabled() || go->GetGOType() != GO_TYPE::ENEMY) continue;
		Enemy* e = dynamic_cast<Enemy*>(go);
		if (!e) continue;

		// Position text above enemy head
		AEVec2 pos = e->GetPos();
		pos.y += e->GetDefinition().render.radius + 20.f;

		std::ostringstream oss;
		oss << e->GetDefinition().name
			<< " HP:" << (int)std::ceilf(e->GetHP())
			<< "/" << (int)std::ceilf(e->GetStats().maxHP);

		DrawAEText(ctx.font, oss.str().c_str(), pos, 0.45f,
			CreateColor(255, 255, 100, 255), TEXT_MIDDLE, 0);
	}
}

// Highlights chest locations with a visual box and label
void DrawChestHighlights(const DebugContext& ctx)
{
	if (!ctx.masterDebugEnabled || !ctx.debugShowChests || ctx.font < 0) return;

	auto& gos = GameObjectManager::GetInstance()->GetGameObjects();
	for (GameObject* go : gos) {
		if (!go || !go->IsEnabled() || go->GetGOType() != GO_TYPE::LOOT_CHEST) continue;

		AEVec2 pos = go->GetPos();
		AEVec2 size = { 80.f, 80.f };

		// Draw gold highlight box
		AEMtx33 mtx;
		GetTransformMtx(mtx, pos, 0, size);
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxTextureSet(nullptr, 0, 0);
		AEGfxSetTransform(mtx.m);
		AEGfxSetColorToMultiply(1.0f, 0.85f, 0.0f, 0.6f);
		AEGfxMeshDraw(ctx.squareMesh, AE_GFX_MDM_TRIANGLES);

		// Draw floating label
		AEVec2 labelPos = { pos.x, pos.y + 55.f };
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		DrawAEText(ctx.font, "CHEST", labelPos, 0.4f,
			CreateColor(255.f, 220.f, 0.f, 255.f), TEXT_MIDDLE, 0);
	}
}

