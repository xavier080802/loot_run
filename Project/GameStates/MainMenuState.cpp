#include "MainMenuState.h"
#include "../Helpers/Vec2Utils.h"
#include "../helpers/CoordUtils.h"
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
