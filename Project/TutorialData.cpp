#include "TutorialData.h"
#include "AEEngine.h"
#include "./Pets/PetManager.h"
#include "./Helpers/CollisionUtils.h"
#include "./DesignPatterns/PostOffice.h"
#include <iostream>
#include "./GameStateManager.h"

namespace {
	const float roomSize = 700.f;
	const float dialogueDur = 3.f;
	const float barrierOffset = roomSize * 0.5f + 160.f;
}

namespace Tutorial {
	void TutorialFairy::InitTutorial(Player* _player, TileMap& _map, MapData& _mapData)
	{
		Init({}, { 50,50 }, 1, MESH_SQUARE, Collision::COL_CIRCLE, {}, 0, Collision::LAYER::NONE)
			->GetRenderData().AddTexture("Assets/fairy.png");

		data.dialogueLines.reserve(4);
		data.dialogueLines.clear();
		player = _player;
		tilemap = &_map;
		mapData = &_mapData;
		SetEnabled(true);
		ChangeStage(START);
		LootChest::SubToChestOpened(this);
		player->SubToGotKill(this);
		SetPos(_map.GetTilePosition(3, 4));
		targetPos = pos;
	}

	void TutorialFairy::Update(double dt)
	{
		float fdt = static_cast<float>(dt);
		AEVec2 pPos = player->GetPos();

		followPlayer = data.isFollowing;
		//Update fairy movement
		Pet::Update(dt);

		//Logic for each stage
		switch (data.stage)
		{
		case Tutorial::START: {
			if (DoDialogue(fdt)) {
				data.playDialogue = false;
				ChangeStage(MOVEMENT);
			}
			break;
		}
		case Tutorial::MOVEMENT: {
			if (data.playDialogue) {
				if (DoDialogue(fdt)) {
					data.playDialogue = false;
				}
				break;
			}

			//Check input
			if (AEInputCheckCurr(AEVK_W) && data.checks[0] != 'w') {
				data.checks[0] = 'w';
				std::cout << data.checks << '\n';
			}
			if (AEInputCheckCurr(AEVK_A) && data.checks[1] != 'a') {
				data.checks[1] = 'a';
				std::cout << data.checks << '\n';
			}
			if (AEInputCheckCurr(AEVK_S) && data.checks[2] != 's') {
				data.checks[2] = 's';
				std::cout << data.checks << '\n';
			}
			if (AEInputCheckCurr(AEVK_D) && data.checks[3] != 'd') {
				data.checks[3] = 'd';
				std::cout << data.checks << '\n';
			}
			//Check if all input done
			if (data.checks == "wasd") {
				ChangeStage(DODGE);
			}
			break;
		}
		case Tutorial::DODGE: {
			if (data.isWaiting) {
				//Go to r2
				if (IsPointOverRectRot(0, 600, roomSize, roomSize,
					0, pPos.x, pPos.y)) {
					data.isWaiting = false;
					data.playDialogue = true;
					std::cout << "WAIT OVER\n";
				}
				break;
			}
			if (data.playDialogue) {
				if (DoDialogue(fdt)) {
					data.playDialogue = false;
					data.isFollowing = true;
				}
				break;
			}
			//Check for dodge
			if (AEInputCheckTriggered(AEVK_SPACE)) {
				ChangeStage(MELEE);
			}
			break;
		}
		case Tutorial::MELEE: {
			if (data.isWaiting) {
				if (data.isWaiting) {
					data.isWaiting = false;
					data.playDialogue = true;
				}
			}
			if (data.playDialogue) {
				if (data.playDialogue && DoDialogue(fdt)) {
					//Dialogue over, move to next room
					data.playDialogue = false;
					tilemap->ChangeTile(3, 14, TileMap::TILE_NONE);
					SetPath({ tilemap->GetTilePosition(3, 18) });
				}
				break;
			}
			//Prevent going backwards
			if (tilemap->GetTileIndFromPos(pPos).x >= 18) {
				tilemap->ChangeTile(3, 14, TileMap::TILE_WALL);
			}
			//Check for kill
			if (data.checks == "M") {
				ChangeStage(LOOT);
			}
			break;
		}
		case Tutorial::LOOT: {
			if (data.isWaiting) {
				//Go to r4
				if (IsPointOverRectRot(800, -600, roomSize, roomSize,
					0, pPos.x, pPos.y)) {
					data.isWaiting = false;
					data.playDialogue = true;
					std::cout << "WAIT OVER\n";
					tilemap->ChangeTile(6, 19, TileMap::TILE_WALL);
				}
				break;
			}
			if (data.playDialogue) {
				if (DoDialogue(fdt)) {
					data.playDialogue = false;
					data.isFollowing = true;
				}
				break;
			}
			//Using subscribers to check if chest is opened.
			//using data.checks for if player opens chest early.
			if (data.checks == "CHEST") {
				ChangeStage(RANGE);
			}
			break;
		}
		case Tutorial::RANGE: {
			if (data.isWaiting) {
				data.playDialogue = true;
				data.isWaiting = false;
			}
			if (data.playDialogue) {
				//Stop before the room until dialogue is done, and player holds their bow.
				if (DoDialogue(fdt) && player->GetHeldWeaponData() && player->GetHeldWeaponData()->isRanged) {
					//Player holding bow, proceed.
					data.playDialogue = false;
					tilemap->ChangeTile(11, 16, TileMap::TILE_NONE);
					SetPath({ tilemap->GetTilePosition(11, 12) });
				}
				else if (!player->GetHeldWeaponData() || !player->GetHeldWeaponData()->isRanged){
					//Player is not holding a bow, wait.
					data.currDialogueLine = 0;
					data.dialogueLines = { "Press Q to equip your bow" };
					data.timer = dialogueDur - 0.5f;
					data.playDialogue = true;
				}
				break;
			}
			//Prevent going backwards
			if (tilemap->GetTileIndFromPos(pPos).x <= 15) {
				tilemap->ChangeTile(16, 11, TileMap::TILE_WALL);
			}
			//Check if enemy is killed in this stage
			if (data.checks == "R") {
				ChangeStage(BOSS);
			}
			break;
		}
		case Tutorial::BOSS: {
			if (data.isWaiting) {
				data.isWaiting = false;
				data.playDialogue = true;
			}
			if (data.playDialogue) {
				if (DoDialogue(fdt)) {
					data.playDialogue = false;
					SetPath({ tilemap->GetTilePosition(11, 5) });
					tilemap->ChangeTile(11, 8, TileMap::TILE_NONE);
				}
				break;
			}
			//Lock player in room with boss
			if (tilemap->GetTileIndFromPos(pPos).x <= 6) {
				tilemap->ChangeTile(11, 8, TileMap::TILE_WALL);
			}
			//Controlled in game state, changes state when boss dies.
			break;
		}
		case Tutorial::END: {
			if (data.isWaiting) {
				data.isWaiting = false;
				data.playDialogue = true;
			}
			data.playDialogue = true;
			if (data.playDialogue) {
				if (DoDialogue(fdt)) {
					data.playDialogue = false;
					data.isFollowing = false;
				}
				break;
			}
			// Walk through the TILE_DOOR tile to exit to main menu.
			if (tilemap && tilemap->IsDoor(pPos)) {
				std::cout << "[Tutorial] Player walked through door - going to main menu.\n";
				GameStateManager::GetInstance()
					->SetNextGameState("MainMenuState", true, true);
			}
			break;
		}
		default:
			break;
		}

		//Dialogue skipping
		if (data.playDialogue && AEInputCheckTriggered(AEVK_RBUTTON)) {
			data.timer = dialogueDur;
		}
	}

	void TutorialFairy::ChangeStage(TUT_STAGE next)
	{
		//Reset common stuff 
		if (next != MOVEMENT) {
			data.isWaiting = true;
			data.isFollowing = false;
			ClearPath();
		}
		data.playDialogue = false;
		data.currDialogueLine = 0;
		data.timer = 0.f;

		//Note: the paths set are the room centers, with the entry direction +- 100 to be closer to entryway
		switch (next)
		{
		case Tutorial::START:
			data.dialogueLines = {"[Skip dialogue with right mouse button]", "Welcome Adventurer to Pandora !", "Battle through various dungeons and gain the strength to defeat evil." };
			data.playDialogue = true;
			break;
		case Tutorial::MOVEMENT:
			data.dialogueLines = { "This is where you learn how to move", "WASD to move up, down, left, and right respectively" };
			data.checks = "----";
			data.playDialogue = true;
			break;
		case Tutorial::DODGE:
			data.dialogueLines = { "If there is an overwhelming horde coming at you, dodge to avoid getting hit", "Press spacebar to use dodge", "Trying dodging and moving simultaneously" };
			SetPath({ tilemap->GetTilePosition(3, 11) });
			break;
		case Tutorial::MELEE:
			data.dialogueLines = { "There's a beast in the next room... Get ready.", "Press left mouse button to attack", "If your weapon is not equipped, press Z." };
			SetPath({ tilemap->GetTilePosition(3, 13) });
			break;
		case Tutorial::LOOT:
			data.dialogueLines = { "Go near the chest and press E to claim your loot", "Press 2 to swap to secondary weapon, press 1 to swap to main weapon"};
			SetPath({ tilemap->GetTilePosition(5, 19), tilemap->GetTilePosition(10, 19) });
			data.checks = "";
			break;
		case Tutorial::RANGE:
			data.dialogueLines = { "Swap to your bow by pressing Q", "Aim your mouse at the enemy and press left mouse button to shoot" };
			SetPath({ tilemap->GetTilePosition(11, 17) });
			break;
		case Tutorial::BOSS:
			data.dialogueLines = { "This is the boss guarding the dungeon, defeat him to clear the dungeon", "Ready.... 3... 2... 1... GO!" };
			SetPath({ tilemap->GetTilePosition(11, 9) });
			break;
		case Tutorial::END:
			data.dialogueLines = { "Brave Adventurer, you have cleared the dungeon !", "Please save Pandora from the Titanomachy." };
			SetPath({ mapData->doorPos });
			break;
		default:
			break;
		}
		std::cout << "CHANGE STAGE ->" << next << '\n';
		data.stage = next;
		SetTutBarrier();
	}

	void TutorialFairy::SubscriptionAlert(LootChestSubContent /*content*/)
	{
		if (data.stage != LOOT) return;

		//Player opens chest before fairy is done - Progress after fairy finishes yapping.
		if (data.playDialogue) {
			data.checks = "CHEST";
			return;
		}
		ChangeStage(RANGE);
	}

	void TutorialFairy::SubscriptionAlert(ActorGotKillSubContent /*content*/)
	{
		if (data.stage == MELEE) {
			data.checks = "M";
			return;
		}
		if (data.stage == RANGE) {
			data.checks = "R";
			return;
		}
	}

	void TutorialFairy::SetTutBarrier()
	{
		if (!tilemap) return;

		switch (data.stage)
		{
		case Tutorial::START:
			tilemap->ChangeTile(3, 6, TileMap::TILE_WALL);
			break;
		case Tutorial::MOVEMENT:
			break;
		case Tutorial::DODGE:
			tilemap->ChangeTile(3, 6, TileMap::TILE_NONE);
			tilemap->ChangeTile(3, 14, TileMap::TILE_WALL);
			break;
		case Tutorial::MELEE:
			tilemap->ChangeTile(3, 14, TileMap::TILE_WALL);
			//tilemap->ChangeTile(6, 19, TileMap::TILE_WALL);
			break;
		case Tutorial::LOOT:
			tilemap->ChangeTile(6, 19, TileMap::TILE_NONE);
			tilemap->ChangeTile(11, 16, TileMap::TILE_WALL);
			break;
		case Tutorial::RANGE:
			//tilemap->ChangeTile(11, 16, TileMap::TILE_NONE);
			tilemap->ChangeTile(11, 8, TileMap::TILE_WALL);
			break;
		case Tutorial::BOSS:
			tilemap->ChangeTile(11, 8, TileMap::TILE_WALL);
			break;
		case Tutorial::END:

		default:
			break;
		}
	}

	//Dialogue Update handling (line duration and moving to next line)
	//Return true if dialogue lines are done
	bool TutorialFairy::DoDialogue(float dt)
	{
		if (!data.playDialogue || data.currDialogueLine >= data.dialogueLines.size()) return true;
		data.timer += dt;
		if (data.timer >= dialogueDur) {
			//Line over
			data.timer = 0.f;
			std::cout << "Line " << data.currDialogueLine + 1 << "/" << data.dialogueLines.size() << " done\n";
			if (data.currDialogueLine == data.dialogueLines.size() - 1) {
				//End of last line - Dialogue done
				return true;
			}

			++data.currDialogueLine;
		}
		return false;
	}
}