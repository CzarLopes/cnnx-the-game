#include <filesystem>
#include <string>
#include <vector>

#include "App.h"
#include "LevelLoader.h"
#include "check.h"

namespace {

Input key(bool Input::*button) {
    Input in;
    in.*button = true;
    return in;
}

// Aperta e solta
void tap(App& app, bool Input::*button) {
    app.step(withPressEdges(key(button), {}));
    app.step({});
}

// Bloco minimo: a porta fica onde o jogador nasce, sem ferramentas
LevelData block(const std::string& id) {
    std::vector<std::string> rows(6, "#........#");
    rows.front() = rows.back() = std::string(10, '#');
    rows[4] = "#..P.....#";
    LevelData level{Room(rows), {}, {}, {48, 48, 16, 32}, {}, 1};
    level.id = id;
    return level;
}

// Abre a porta, monta o puzzle e confirma a tela de bloco concluido
void finishBlock(App& app) {
    app.step({});
    tap(app, &Input::up);
    for (int slot = 0; slot < app.game()->puzzle().wireCount(); ++slot) {
        if (app.game()->puzzle().wireAt(slot) == slot) {
            continue;
        }
        int from = slot;
        while (app.game()->puzzle().wireAt(from) != slot) {
            ++from;
        }
        while (app.game()->puzzle().cursor() != from) {
            tap(app, app.game()->puzzle().cursor() > from ? &Input::left : &Input::right);
        }
        tap(app, &Input::jumpHeld);
        while (app.game()->puzzle().cursor() != slot) {
            tap(app, &Input::left);
        }
        tap(app, &Input::jumpHeld);
    }
    tap(app, &Input::shootHeld);
    tap(app, &Input::jumpHeld);
}

void testWorldAndPhaseSelect(const std::string& save) {
    App app({block("w1_f1_b2"), block("w1_f1_b1"), block("w1_f2_b1")}, save, 1);
    CHECK(app.screen() == AppScreen::Title && !app.title().hasSave(), "comeca na tela inicial sem save");

    tap(app, &Input::jumpHeld);
    CHECK(app.screen() == AppScreen::WorldSelect && app.world() == 0, "novo jogo abre o mapa no Planeta Catraca");
    tap(app, &Input::right);
    tap(app, &Input::jumpHeld);
    CHECK(app.screen() == AppScreen::WorldSelect && app.world() == 1, "mundo fechado nao abre");
    tap(app, &Input::left);
    tap(app, &Input::jumpHeld);
    CHECK(app.screen() == AppScreen::PhaseSelect && app.phaseSelect().cursor() == 0, "Planeta Catraca abre as fases");

    CHECK(app.phaseState(0, 0) == PhaseState::Open && app.blockCount(0, 0) == 2, "fase 1 aberta com 2 blocos");
    CHECK(app.phaseState(0, 1) == PhaseState::Locked, "fase 2 espera a fase 1");
    CHECK(app.phaseState(0, 2) == PhaseState::Soon && app.phaseState(0, kFinalPhase) == PhaseState::Soon, "fases sem blocos ficam em construcao");
    CHECK(app.phaseState(1, 0) == PhaseState::Soon, "mundos fechados nao tem fases");

    tap(app, &Input::right);
    tap(app, &Input::jumpHeld);
    CHECK(app.screen() == AppScreen::PhaseSelect, "fase bloqueada nao abre");
    tap(app, &Input::left);
    tap(app, &Input::jumpHeld);
    CHECK(app.screen() == AppScreen::Playing && app.game()->levelIndex() == 0, "fase 1 comeca no primeiro bloco, em ordem de id");

    finishBlock(app);
    CHECK(app.screen() == AppScreen::Playing && app.game()->levelIndex() == 1, "concluir o bloco leva ao proximo da fase");
    CHECK(app.back() && app.screen() == AppScreen::PhaseSelect, "Esc no jogo volta para as fases");
    CHECK(app.profile().savedLevel() == "w1_f1_b2", "o bloco atual fica salvo");

    tap(app, &Input::jumpHeld);
    CHECK(app.screen() == AppScreen::Playing && app.game()->levelIndex() == 1, "voltar a fase continua do bloco salvo");
    finishBlock(app);
    CHECK(app.screen() == AppScreen::PhaseSelect && app.phaseSelect().cursor() == 1, "concluir a fase volta para as fases com o cursor na proxima");
    CHECK(app.profile().isPhaseComplete("w1_f1") && app.profile().savedLevel().empty(), "fase concluida fica salva");
    CHECK(app.phaseState(0, 0) == PhaseState::Complete && app.phaseState(0, 1) == PhaseState::Open, "concluir a fase 1 libera a fase 2");

    CHECK(app.back() && app.screen() == AppScreen::WorldSelect, "Esc nas fases volta ao mapa");
    tap(app, &Input::dashHeld);
    CHECK(app.screen() == AppScreen::Title && app.title().hasSave(), "X no mapa volta para a tela inicial, agora com save");
    CHECK(!app.back(), "Esc na tela inicial pede para fechar o jogo");
}

void testLoadGame(const std::string& save) {
    App app({block("w1_f1_b1"), block("w1_f1_b2"), block("w1_f2_b1")}, save, 1);
    CHECK(app.title().hasSave() && app.title().cursor() == TitleMenu::kLoadGame, "o save do teste anterior aparece no menu");
    tap(app, &Input::jumpHeld);
    CHECK(app.screen() == AppScreen::WorldSelect && app.world() == 0, "carregar abre o mapa");
    tap(app, &Input::jumpHeld);
    CHECK(app.phaseSelect().cursor() == 1, "o cursor vai para a primeira fase ainda por jogar");
}

void testRealProject() {
    const std::vector<LevelData> levels = loadAllLevels(CNNX_LEVEL_PATH);
    CHECK(levels.size() >= 2 && phaseOfLevel(levels[0].id) == "w1_f1", "o projeto carrega todos os blocos da fase 1 do Planeta Catraca");
    CHECK(phaseId(0, 0) == "w1_f1" && phaseOfLevel("w1_f2_b3") == "w1_f2" && phaseOfLevel("sem_padrao").empty(), "ids de fase");
}

}  // namespace

void runAppTests() {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "cnnx_teste_app" / "save.json";
    std::filesystem::remove_all(path.parent_path());
    testWorldAndPhaseSelect(path.string());
    testLoadGame(path.string());
    std::filesystem::remove_all(path.parent_path());
    testRealProject();
}
