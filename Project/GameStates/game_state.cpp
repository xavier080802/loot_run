#include "game_state.h"
#include <iostream>

void GameState::InitState()
{
}

void GameState::EnterState()
{
	std::cout << "Game state enter\n";
}

void GameState::ExitState()
{
}

void GameState::UnloadState()
{
}

void GameState::Update(double dt)
{
	std::cout << "Game state update " << dt << '\n';
}
