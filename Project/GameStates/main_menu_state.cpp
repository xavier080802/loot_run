#include "main_menu_state.h"
#include "../GameObjects/GameObject.h"
#include "../Helpers/vec2_utils.h"
#include <iostream>

static GameObject* go;

static void AnimEvent(unsigned frame, bool isDone) {
	std::cout << "Frame: " << frame << " | is last?: " << std::boolalpha << isDone << '\n';
}

void MainMenuState::InitState()
{
	go = new GameObject;
	go->Init(VecZero(), { 100,100 }, MESH_SQUARE_ANIM, COL_RECT, { 100,100 }, 0);
	go->GetRenderData().InitAnimation(6, 9)
		->LoopAnim()
		->SetFrameCallback(AnimEvent)
		->AddTexture("Assets/sprite_test.png");
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
	go->Free();
	delete go;
}

void MainMenuState::Update(float dt)
{
	//std::cout << "Main menu state update " << dt << '\n';
	go->Update(dt);
	go->Draw();
}

MainMenuState::MainMenuState()
{
	InitState();
}
