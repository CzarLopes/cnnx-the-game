#include "TitleMenu.h"

TitleMenu::TitleMenu(bool hasSave) : hasSave_(hasSave), cursor_(hasSave ? kLoadGame : kNewGame) {}

bool TitleMenu::enabled(int option) const {
    return option != kLoadGame || hasSave_;
}

const char* TitleMenu::optionLabel(int option) {
    switch (option) {
        case kNewGame: return "Novo jogo";
        case kLoadGame: return "Carregar jogo";
        case kSettings: return "Configuracoes";
        default: return "";
    }
}

TitleAction TitleMenu::step(const Input& in) {
    ++frame_;
    switch (screen_) {
        case Screen::Main:
            if (in.upPressed) {
                move(-1);
            }
            if (in.downPressed) {
                move(1);
            }
            return in.jumpPressed ? confirm() : TitleAction::None;
        case Screen::ConfirmNewGame:
            if (in.jumpPressed) {
                screen_ = Screen::Main;
                return TitleAction::NewGame;
            }
            if (in.dashPressed) {
                back();
            }
            return TitleAction::None;
        case Screen::Settings:
            if (in.jumpPressed || in.dashPressed) {
                back();
            }
            return TitleAction::None;
    }
    return TitleAction::None;
}

bool TitleMenu::back() {
    if (screen_ == Screen::Main) {
        return false;
    }
    screen_ = Screen::Main;
    return true;
}

void TitleMenu::move(int direction) {
    do {
        cursor_ = (cursor_ + direction + kOptionCount) % kOptionCount;
    } while (!enabled(cursor_));
}

TitleAction TitleMenu::confirm() {
    switch (cursor_) {
        case kNewGame:
            // Com progresso salvo, pede confirmacao antes de apagar
            if (hasSave_) {
                screen_ = Screen::ConfirmNewGame;
                return TitleAction::None;
            }
            return TitleAction::NewGame;
        case kLoadGame:
            return TitleAction::LoadGame;
        case kSettings:
            screen_ = Screen::Settings;
            return TitleAction::None;
    }
    return TitleAction::None;
}
