#include "game_state.h"
#include <iostream>

void GameState::LoadState()
{
}

void GameState::InitState()
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

void GameState::Draw()
{
}
