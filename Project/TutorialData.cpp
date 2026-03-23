#include "TutorialData.h"
#include "AEEngine.h"
#include "./Pets/PetManager.h"
#include "./Helpers/CollisionUtils.h"
#include "./DesignPatterns/PostOffice.h"
#include <iostream>
#include "./GameStateManager.h"

namespace {
	const float roomSize = 700.f;
	const float dialogueDur = 2.f;
	const float barrierOffset = roomSize * 0.5f + 160.f;
}

namespace Tutorial {
	void TutorialFairy::InitTutorial(Player* _player, MapData* _map)
	{
		Init({}, { 50,50 }, 1, MESH_SQUARE, Collision::COL_CIRCLE, {}, 0, Collision::LAYER::NONE)
			->GetRenderData().AddTexture("Assets/fairy.png");

		data.dialogueLines.reserve(4);
		data.dialogueLines.clear();
		player = _player;
		map = _map;
		SetEnabled(true);
		ChangeStage(START);
		LootChest::SubToChestOpened(this);
		SetPos({ map->startPos.x + roomSize / 2.f, map->startPos.y });
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
				//Go to r3
				if (IsPointOverRectRot(800, 600, roomSize, roomSize,
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
			//Check melee attack input
			if (AEInputCheckTriggered(AEVK_LBUTTON)) {
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
				//Go to r5
				if (IsPointOverRectRot(0, -600, roomSize, roomSize,
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
			//Check if range weapon is used (TEMP: havent checked for what is the active weapon)
			if (AEInputCheckTriggered(AEVK_LBUTTON)) {
				ChangeStage(BOSS);
			}
			break;
		}
		case Tutorial::BOSS: {
			if (data.isWaiting) {
				//Go to r6
				if (IsPointOverRectRot(-800, -600, roomSize, roomSize,
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
			//Controlled in game state, changes state when boss dies.
			break;
		}
		case Tutorial::END: {
			if (data.playDialogue) {
				if (DoDialogue(fdt)) {
					data.playDialogue = false;
					data.isFollowing = false;
				}
				break;
			}
			// Walk through the TILE_DOOR tile to exit to main menu.
			if (tilemap && tilemap->IsDoor(pPos)) {
				std::cout << "[Tutorial] Player walked through door � going to main menu.\n";
				GameStateManager::GetInstance()
					->SetNextGameState("MainMenuState", true, true);
			}
			break;
		}
		default:
			break;
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
			data.dialogueLines = { "START dialogue line 1/2", "START dialogue line 2/2" };
			data.playDialogue = true;
			break;
		case Tutorial::MOVEMENT:
			data.dialogueLines = { "MOVEMENT dialogue line 1/2", "MOVEMENT dialogue line 2/2" };
			data.checks = "----";
			data.playDialogue = true;
			break;
		case Tutorial::DODGE:
			data.dialogueLines = { "DODGE 1/3", "DODGE dialogue line 2/3", "DODGE line 3/3" };
			SetPath({ {0,600} });
			break;
		case Tutorial::MELEE:
			data.dialogueLines = { "MELEE dialogue line 1/2", "MELEE dialogue line 2/2" };
			SetPath({ {700,600} });
			break;
		case Tutorial::LOOT:
			data.dialogueLines = { "LOOT dialogue line 1/1" };
			SetPath({ {800, -500} });
			data.checks = "";
			break;
		case Tutorial::RANGE:
			data.dialogueLines = { "RANGE dialogue line 1/2", "RANGE dialogue line 2/2" };
			SetPath({ {100,-600} });
			break;
		case Tutorial::BOSS:
			data.dialogueLines = { "BOSS dialogue line 1/2", "BOSS dialogue line 2/2" };
			SetPath({ {-700,-600} });
			break;
		case Tutorial::END:
			data.dialogueLines = { "END dialogue line 1/2", "END dialogue line 2/2" };
			SetPath({ map->doorPos });
			break;
		default:
			break;
		}
		std::cout << "CHANGE STAGE ->" << next << '\n';
		data.stage = next;
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

	AEVec2 TutorialFairy::GetTutBarrier()
	{
		switch (data.stage)
		{
		case Tutorial::START:
		case Tutorial::MOVEMENT:
			return { -800 + barrierOffset, 600 };
		case Tutorial::DODGE:
			return { barrierOffset, 600 };
		case Tutorial::MELEE:
			return { 800, 600 - barrierOffset };
		case Tutorial::LOOT:
			return { 800 - barrierOffset, -600 };
		case Tutorial::RANGE:
			return { -barrierOffset, -600 };

		case Tutorial::BOSS:
		case Tutorial::END:

		default:
			return { 2000,2000 }; //arbitrary large number
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