#include "main_menu_state.h"
#include "../Helpers/vec2_utils.h"
#include "../helpers/coord_utils.h"
#include <iostream>

void MainMenuState::LoadState()
{
}

void MainMenuState::InitState()
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
	UNREFERENCED_PARAMETER(dt);
}

void MainMenuState::Draw()
{
}
