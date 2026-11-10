#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "Game.h"
#include "Input.h"
#include "LevelLoader.h"
#include "Profile.h"
#include "Selector.h"
#include "TitleMenu.h"
#include "Worlds.h"

enum class AppScreen { Title, WorldSelect, PhaseSelect, Playing };

// Soon: ainda sem blocos. Locked: espera a fase anterior. Open e Complete podem ser jogadas.
enum class PhaseState { Soon, Locked, Open, Complete };

// Fluxo entre telas: inicial -> mapa de mundos -> fases -> jogo. Nao depende de raylib.
class App {
public:
    // levels: todos os blocos do projeto; savePath vazio nao grava (testes).
    App(std::vector<LevelData> levels, std::string savePath, unsigned seed);

    void step(const Input& in);
    // Volta uma tela (Esc). Retorna false na tela inicial, onde Esc fecha o jogo.
    bool back();

    AppScreen screen() const { return screen_; }
    const TitleMenu& title() const { return title_; }
    const Selector& worldSelect() const { return worldSelect_; }
    const Selector& phaseSelect() const { return phaseSelect_; }
    Game* game() { return game_ ? &*game_ : nullptr; }
    const Game* game() const { return game_ ? &*game_ : nullptr; }
    const Profile& profile() const { return profile_; }

    // Mundo aberto na tela de fases (o do cursor no mapa).
    int world() const { return worldSelect_.cursor(); }
    PhaseState phaseState(int world, int phase) const;
    int blockCount(int world, int phase) const;

private:
    void stepTitle(const Input& in);
    void stepWorldSelect(const Input& in);
    void stepPhaseSelect(const Input& in);
    void stepPlaying(const Input& in);
    void openWorldSelect();
    void openPhaseSelect(int phase);
    void startPhase(int phase);
    void leaveGame();
    Profile loadProfile() const;

    std::map<std::string, std::vector<LevelData>> phases_;
    std::string savePath_;
    unsigned seed_;
    Profile profile_;
    AppScreen screen_ = AppScreen::Title;
    TitleMenu title_;
    Selector worldSelect_;
    Selector phaseSelect_;
    std::optional<Game> game_;
    int playingPhase_ = 0;
};
