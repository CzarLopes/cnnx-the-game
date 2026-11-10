#include "App.h"

#include <algorithm>
#include <utility>

namespace {

// Mundo do bloco salvo, ou -1 se nao houver
int worldOfLevel(const std::string& levelId) {
    for (int world = 0; world < kWorldCount; ++world) {
        if (levelId.rfind(std::string(worlds()[world].id) + "_", 0) == 0) {
            return world;
        }
    }
    return -1;
}

}  // namespace

App::App(std::vector<LevelData> levels, std::string savePath, unsigned seed)
    : savePath_(std::move(savePath)), seed_(seed), profile_(loadProfile()), title_(profile_.hasProgress()) {
    for (LevelData& level : levels) {
        const std::string phase = phaseOfLevel(level.id);
        if (!phase.empty()) {
            phases_[phase].push_back(std::move(level));
        }
    }
    for (auto& [phase, blocks] : phases_) {
        std::sort(blocks.begin(), blocks.end(), [](const LevelData& a, const LevelData& b) { return a.id < b.id; });
    }
}

Profile App::loadProfile() const {
    return savePath_.empty() ? Profile() : Profile(savePath_);
}

PhaseState App::phaseState(int world, int phase) const {
    if (!worlds()[world].open) {
        return PhaseState::Soon;
    }
    const std::string id = phaseId(world, phase);
    if (phases_.find(id) == phases_.end()) {
        return PhaseState::Soon;
    }
    if (profile_.isPhaseComplete(id)) {
        return PhaseState::Complete;
    }
    // A fase final pede todas as outras; as demais, so a anterior
    const int first = phase == kFinalPhase ? 0 : phase - 1;
    for (int previous = std::max(first, 0); previous < phase; ++previous) {
        if (!profile_.isPhaseComplete(phaseId(world, previous))) {
            return PhaseState::Locked;
        }
    }
    return PhaseState::Open;
}

int App::blockCount(int world, int phase) const {
    const auto found = phases_.find(phaseId(world, phase));
    return found == phases_.end() ? 0 : static_cast<int>(found->second.size());
}

void App::step(const Input& in) {
    switch (screen_) {
        case AppScreen::Title: stepTitle(in); break;
        case AppScreen::WorldSelect: stepWorldSelect(in); break;
        case AppScreen::PhaseSelect: stepPhaseSelect(in); break;
        case AppScreen::Playing: stepPlaying(in); break;
    }
}

bool App::back() {
    switch (screen_) {
        case AppScreen::Title:
            return title_.back();
        case AppScreen::WorldSelect:
            title_ = TitleMenu(profile_.hasProgress());
            screen_ = AppScreen::Title;
            return true;
        case AppScreen::PhaseSelect:
            screen_ = AppScreen::WorldSelect;
            return true;
        case AppScreen::Playing:
            leaveGame();
            return true;
    }
    return false;
}

void App::stepTitle(const Input& in) {
    switch (title_.step(in)) {
        case TitleAction::NewGame:
            profile_ = loadProfile();
            profile_.reset();
            openWorldSelect();
            break;
        case TitleAction::LoadGame:
            profile_ = loadProfile();
            openWorldSelect();
            break;
        case TitleAction::None:
            break;
    }
}

void App::openWorldSelect() {
    int cursor = worldOfLevel(profile_.savedLevel());
    if (cursor < 0) {
        const auto& list = worlds();
        cursor = static_cast<int>(std::find_if(list.begin(), list.end(), [](const WorldInfo& w) { return w.open; }) - list.begin());
    }
    worldSelect_ = Selector(kWorldCount, cursor);
    screen_ = AppScreen::WorldSelect;
}

void App::stepWorldSelect(const Input& in) {
    switch (worldSelect_.step(in)) {
        case SelectResult::Confirm: {
            if (!worlds()[world()].open) {
                break;
            }
            // Cursor na fase do bloco salvo ou na primeira fase ainda por jogar
            const std::string saved = phaseOfLevel(profile_.savedLevel());
            int phase = 0;
            for (int candidate = kFinalPhase; candidate >= 0; --candidate) {
                if (phaseId(world(), candidate) == saved) {
                    phase = candidate;
                    break;
                }
                if (phaseState(world(), candidate) == PhaseState::Open) {
                    phase = candidate;
                }
            }
            openPhaseSelect(phase);
            break;
        }
        case SelectResult::Back:
            back();
            break;
        case SelectResult::None:
            break;
    }
}

void App::openPhaseSelect(int phase) {
    phaseSelect_ = Selector(kPhasesPerWorld, phase);
    screen_ = AppScreen::PhaseSelect;
}

void App::stepPhaseSelect(const Input& in) {
    switch (phaseSelect_.step(in)) {
        case SelectResult::Confirm: {
            const PhaseState state = phaseState(world(), phaseSelect_.cursor());
            if (state == PhaseState::Open || state == PhaseState::Complete) {
                startPhase(phaseSelect_.cursor());
            }
            break;
        }
        case SelectResult::Back:
            back();
            break;
        case SelectResult::None:
            break;
    }
}

void App::startPhase(int phase) {
    const std::string id = phaseId(world(), phase);
    const std::vector<LevelData>& blocks = phases_.at(id);
    // Volta ao bloco onde parou se ele for desta fase
    const size_t start = phaseOfLevel(profile_.savedLevel()) == id ? findLevel(blocks, profile_.savedLevel()) : 0;
    game_.emplace(blocks, profile_, seed_++, start);
    playingPhase_ = phase;
    screen_ = AppScreen::Playing;
}

void App::stepPlaying(const Input& in) {
    game_->step(in);
    if (game_->mode() != GameMode::Finished) {
        return;
    }
    profile_ = game_->profile();
    profile_.completePhase(phaseId(world(), playingPhase_));
    game_.reset();
    const int next = std::min(playingPhase_ + 1, kFinalPhase);
    openPhaseSelect(phaseState(world(), next) == PhaseState::Open ? next : playingPhase_);
}

void App::leaveGame() {
    profile_ = game_->profile();
    game_.reset();
    openPhaseSelect(playingPhase_);
}
