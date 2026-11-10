#pragma once

#include <string>

#include "Game.h"
#include "TitleMenu.h"

constexpr int kViewWidth = 256;
constexpr int kViewHeight = 224;

// Precisam de janela aberta. O load lanca std::runtime_error se faltar arquivo.
void loadGameTextures(const std::string& assetDirectory);
void unloadGameTextures();

// Desenha uma tela de kViewWidth x kViewHeight no alvo de desenho atual.
void drawGame(const Game& game);
void drawTitle(const TitleMenu& menu);
