#include "Game.h"

#include <algorithm>
#include <utility>

namespace {

constexpr int kDeathFrames = 60;
constexpr int kMessageFrames = 120;

float centerX(const Box& box) {
    return box.x + box.w / 2.0f;
}

// Empurra o jogador para longe da origem do dano
int knockDirection(const Box& player, const Box& source) {
    return centerX(player) >= centerX(source) ? 1 : -1;
}

}  // namespace

Game::Game(LevelData level)
    : level_(std::move(level)),
      hazards_(level_.room, level_.darkZones),
      strength_(level_.strength) {
    respawn();
}

void Game::respawn() {
    player_.spawn(level_.room.spawnX(), level_.room.spawnFeetY());
    shots_.clear();
    enemyShots_.clear();
    enemies_.clear();
    for (const EnemySpawn& spawn : level_.enemies) {
        enemies_.push_back(makeEnemy(spawn, strength_));
    }
    hazards_.reset(level_.room);
    deathTimer_ = 0;
    messageTimer_ = 0;
}

void Game::setStrength(int strength) {
    strength_ = std::clamp(strength, kMinStrength, kMaxStrength);
    respawn();
}

void Game::restartBlock() {
    for (ToolPickup& tool : level_.tools) {
        tool.collected = false;
    }
    doorOpen_ = false;
    mode_ = GameMode::Playing;
    respawn();
}

int Game::toolsMissing() const {
    return static_cast<int>(std::count_if(level_.tools.begin(), level_.tools.end(),
                                          [](const ToolPickup& tool) { return !tool.collected; }));
}

void Game::step(const Input& in) {
    ++frame_;
    switch (mode_) {
        case GameMode::Playing:
            stepPlaying(in);
            break;
        case GameMode::Puzzle:
            stepPuzzle(in);
            break;
        case GameMode::Complete:
            if (in.jumpPressed) {
                restartBlock();
            }
            break;
    }
}

void Game::stepPlaying(const Input& in) {
    messageTimer_ = std::max(messageTimer_ - 1, 0);

    player_.update(in, level_.room, shots_);
    updateShots(shots_, level_.room);
    hazards_.update(level_.room, player_.bounds(), player_.grounded());
    updateEnemies(enemies_, enemyShots_, level_.room, player_.bounds(), strength_);
    updateEnemyShots(enemyShots_, level_.room);
    resolveCombat();

    if (player_.isDead()) {
        if (++deathTimer_ >= kDeathFrames) {
            respawn();
        }
        return;
    }

    const int hazardDamage = hazards_.damageAt(level_.room, player_.bounds());
    if (hazardDamage > 0) {
        player_.takeDamage(hazardDamage, -player_.facing());
    }
    collectTools();
    tryOpenDoor(in);
}

void Game::resolveCombat() {
    for (auto shot = shots_.begin(); shot != shots_.end();) {
        const Box box = shotBox(*shot);
        auto hit = std::find_if(enemies_.begin(), enemies_.end(),
                                [&box](const Enemy& enemy) { return boxesOverlap(box, enemy.box); });
        if (hit == enemies_.end()) {
            ++shot;
            continue;
        }
        hit->hp -= shotDamage(*shot);
        if (hit->hp <= 0) {
            enemies_.erase(hit);
        }
        shot = shots_.erase(shot);
    }

    const Box playerBox = player_.bounds();
    const int contactDamage = forceStats(strength_).contactDamage;
    for (const Enemy& enemy : enemies_) {
        if (boxesOverlap(playerBox, enemy.box)) {
            player_.takeDamage(contactDamage, knockDirection(playerBox, enemy.box));
        }
    }
    enemyShots_.erase(std::remove_if(enemyShots_.begin(), enemyShots_.end(),
                                     [this, &playerBox](const EnemyShot& shot) {
                                         return boxesOverlap(playerBox, shot.box) &&
                                                player_.takeDamage(shot.damage,
                                                                   knockDirection(playerBox, shot.box));
                                     }),
                      enemyShots_.end());
}

void Game::collectTools() {
    const Box playerBox = player_.bounds();
    for (ToolPickup& tool : level_.tools) {
        if (!tool.collected && boxesOverlap(playerBox, tool.box)) {
            tool.collected = true;
        }
    }
}

void Game::tryOpenDoor(const Input& in) {
    if (!in.upPressed || !boxesOverlap(player_.bounds(), level_.door)) {
        return;
    }
    if (toolsMissing() > 0) {
        messageTimer_ = kMessageFrames;
        return;
    }
    puzzle_ = PuzzleRj45(static_cast<unsigned>(frame_));
    mode_ = GameMode::Puzzle;
}

void Game::stepPuzzle(const Input& in) {
    if (in.leftPressed) {
        puzzle_.moveCursor(-1);
    }
    if (in.rightPressed) {
        puzzle_.moveCursor(1);
    }
    if (in.jumpPressed) {
        puzzle_.select();
    }
    if (in.shootPressed) {
        puzzle_.test();
    }
    if (puzzle_.status() == PuzzleRj45::Status::Solved) {
        doorOpen_ = true;
        mode_ = GameMode::Complete;
    }
}
