#pragma once

#include "Game.h"

constexpr int kViewWidth = 256;
constexpr int kViewHeight = 224;

// Desenha uma tela de kViewWidth x kViewHeight no alvo de desenho atual.
void drawGame(const Game& game);
