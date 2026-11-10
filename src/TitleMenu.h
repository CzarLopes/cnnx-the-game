#pragma once

#include "Input.h"

enum class TitleAction { None, NewGame, LoadGame };

// Menu da tela inicial. Cima e baixo escolhem, Z confirma e X volta.
class TitleMenu {
public:
    enum class Screen { Main, ConfirmNewGame, Settings };
    enum Option { kNewGame, kLoadGame, kSettings, kOptionCount };

    // Sem save, "Carregar jogo" fica desativado e o cursor pula essa opcao.
    explicit TitleMenu(bool hasSave);

    TitleAction step(const Input& in);
    // Volta para o menu principal. Retorna false se ja estava nele.
    bool back();

    Screen screen() const { return screen_; }
    int cursor() const { return cursor_; }
    bool hasSave() const { return hasSave_; }
    bool enabled(int option) const;
    int frame() const { return frame_; }
    static const char* optionLabel(int option);

private:
    void move(int direction);
    TitleAction confirm();

    bool hasSave_;
    Screen screen_ = Screen::Main;
    int cursor_;
    int frame_ = 0;
};
