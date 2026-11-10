#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "Game.h"
#include "LevelLoader.h"
#include "Profile.h"
#include "TitleMenu.h"
#include "check.h"

namespace {

// Aperta e solta, como uma pessoa no menu
TitleAction tap(TitleMenu& menu, Input held) {
    const TitleAction action = menu.step(withPressEdges(held, {}));
    menu.step({});
    return action;
}

Input upKey() {
    Input in;
    in.up = true;
    return in;
}

Input downKey() {
    Input in;
    in.down = true;
    return in;
}

Input confirmKey() {
    Input in;
    in.jumpHeld = true;
    return in;
}

Input backKey() {
    Input in;
    in.dashHeld = true;
    return in;
}

void testMenuWithoutSave() {
    TitleMenu menu(false);
    CHECK(menu.cursor() == TitleMenu::kNewGame && !menu.enabled(TitleMenu::kLoadGame), "sem save o cursor comeca em Novo jogo e Carregar fica desativado");
    tap(menu, downKey());
    CHECK(menu.cursor() == TitleMenu::kSettings, "descer pula o Carregar desativado");
    tap(menu, downKey());
    CHECK(menu.cursor() == TitleMenu::kNewGame, "o cursor da a volta no fim da lista");
    tap(menu, upKey());
    tap(menu, upKey());
    CHECK(menu.cursor() == TitleMenu::kNewGame, "subir tambem pula o Carregar");
    CHECK(tap(menu, confirmKey()) == TitleAction::NewGame, "sem save, Novo jogo comeca direto");
}

void testMenuWithSave() {
    TitleMenu menu(true);
    CHECK(menu.cursor() == TitleMenu::kLoadGame, "com save o cursor comeca em Carregar jogo");
    CHECK(tap(menu, confirmKey()) == TitleAction::LoadGame, "Z em Carregar jogo carrega");

    TitleMenu fresh(true);
    tap(fresh, upKey());
    CHECK(tap(fresh, confirmKey()) == TitleAction::None && fresh.screen() == TitleMenu::Screen::ConfirmNewGame, "com save, Novo jogo pede confirmacao");
    tap(fresh, backKey());
    CHECK(fresh.screen() == TitleMenu::Screen::Main && fresh.cursor() == TitleMenu::kNewGame, "X cancela e volta ao menu");
    tap(fresh, confirmKey());
    CHECK(tap(fresh, confirmKey()) == TitleAction::NewGame, "confirmar comeca o novo jogo");
}

void testSettings() {
    TitleMenu menu(false);
    tap(menu, upKey());
    CHECK(tap(menu, confirmKey()) == TitleAction::None && menu.screen() == TitleMenu::Screen::Settings, "Configuracoes abre a propria tela");
    tap(menu, backKey());
    CHECK(menu.screen() == TitleMenu::Screen::Main, "X volta das configuracoes");
    CHECK(!menu.back(), "back no menu principal avisa que ja esta nele");
    tap(menu, confirmKey());
    CHECK(menu.back() && menu.screen() == TitleMenu::Screen::Main, "back sai das configuracoes");
}

LevelData namedArena(const std::string& id) {
    std::vector<std::string> rows(6, "#........#");
    rows.front() = rows.back() = std::string(10, '#');
    rows[4] = "#..P.....#";
    LevelData level{Room(rows), {}, {}, {120, 48, 16, 32}, {}, 1};
    level.id = id;
    return level;
}

void testSavedProgress() {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "cnnx_teste_progresso" / "save.json";
    std::filesystem::remove_all(path.parent_path());

    CHECK(!Profile(path.string()).hasProgress(), "sem arquivo nao ha progresso");

    std::vector<LevelData> levels = {namedArena("w1_f1_b1"), namedArena("w1_f1_b2")};
    CHECK(findLevel(levels, "w1_f1_b2") == 1 && findLevel(levels, "nao_existe") == 0, "findLevel acha o bloco ou cai no primeiro");

    {
        Profile profile(path.string());
        profile.addUpgrade("segredo");
        const Game game(levels, std::move(profile), 1, 1);
        CHECK(game.levelIndex() == 1, "o jogo comeca no bloco pedido");
    }
    const Profile saved(path.string());
    CHECK(saved.hasProgress() && saved.savedLevel() == "w1_f1_b2" && saved.hasUpgrade("segredo"), "entrar no bloco grava ele no save junto das conquistas");

    Profile reset(path.string());
    reset.reset();
    const Profile cleared(path.string());
    CHECK(!cleared.hasProgress() && cleared.maxHp() == Profile::kBaseMaxHp, "novo jogo apaga bloco e conquistas do arquivo");

    const Game fresh(levels, Profile(path.string()), 1, findLevel(levels, cleared.savedLevel()));
    CHECK(fresh.levelIndex() == 0 && Profile(path.string()).savedLevel() == "w1_f1_b1", "novo jogo comeca e grava o primeiro bloco");
    std::filesystem::remove_all(path.parent_path());
}

}  // namespace

void runMenuTests() {
    testMenuWithoutSave();
    testMenuWithSave();
    testSettings();
    testSavedProgress();
}
