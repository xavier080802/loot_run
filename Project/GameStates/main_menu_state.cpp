#include "main_menu_state.h"
#include <iostream>
void MainMenuState::InitState()
{
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

void MainMenuState::Update(float dt)
{
	std::cout << "Main menu state update " << dt << '\n';
}

MainMenuState::MainMenuState()
{
	InitState();
}
