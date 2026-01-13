#include "main_menu_state.h"
#include "../GameObjects/GameObject.h"
#include "../Helpers/vec2_utils.h"
#include "../helpers/coord_utils.h"
#include <iostream>

static GameObject* go;
static GameObject* enemy;

static void AnimEvent(unsigned frame, bool isDone) {
	std::cout << "Frame event: " << frame << " is last?: " << std::boolalpha << isDone << '\n';
}

void MainMenuState::InitState()
{
	//Testing
	GameObjectManager::GetInstance()->InitCollisionGrid(1600, 800);
	go = new GameObject;
	go->Init(VecZero(), { 100,100 }, 0, MESH_SQUARE_ANIM, COL_RECT, { 100,100 }, CreateBitmask(1, GameObject::ENEMIES), GameObject::COLLISION_LAYER::PLAYER);
	go->GetRenderData().InitAnimation(6, 9)
		->LoopAnim()
		->SetFrameCallback(AnimEvent)
		->AddTexture("Assets/sprite_test.png");

	enemy = new GameObject;
	enemy->Init({ 150,0 }, { 100,-100 }, 0, MESH_CIRCLE, COL_CIRCLE, { 100,100 }, CreateBitmask(1, GameObject::PLAYER), GameObject::ENEMIES);
}

void MainMenuState::EnterState()
{
	std::cout << "Main menu state enter\n";
}

void MainMenuState::ExitState()
{
	std::cout << "Exit main menu state\n";
}

void MainMenuState::UnloadState()
{

}

void MainMenuState::Update(double dt)
{
	//std::cout << "Main menu state update " << dt << '\n';
	if (AEInputCheckCurr(AEVK_E)) {
		s32 x, y;
		AEInputGetCursorPosition(&x, &y);
		enemy->GetPos() = ScreenToWorld(ToVec2(x, y));
	}
}
