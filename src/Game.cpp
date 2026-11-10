#include "Game.h"

#include <algorithm>
#include <optional>
#include <utility>

namespace {

constexpr int kMessageFrames = 120;

float centerX(const Box& box) {
    return box.x + box.w / 2.0f;
}

// Empurra o jogador para longe da origem do dano
int knockDirection(const Box& player, const Box& source) {
    return centerX(player) >= centerX(source) ? 1 : -1;
}

}  // namespace

Game::Game(LevelData level, Profile profile, unsigned seed)
    : Game(std::vector<LevelData>{std::move(level)}, std::move(profile), seed) {}

Game::Game(std::vector<LevelData> levels, Profile profile, unsigned seed, size_t startLevel)
    : levels_(std::move(levels)),
      level_(levels_.at(0)),
      hazards_(level_.room, level_.darkZones, level_.fallingObjects),
      profile_(std::move(profile)),
      random_(seed) {
    enterLevel(startLevel < levels_.size() ? startLevel : 0);
}

void Game::enterLevel(size_t index) {
    levelIndex_ = index;
    level_ = levels_[index];
    if (!level_.id.empty()) {
        profile_.setLevel(level_.id);
    }
    hazards_ = Hazards(level_.room, level_.darkZones, level_.fallingObjects);
    strength_ = level_.strength;
    doorOpen_ = false;
    mode_ = GameMode::Playing;
    respawn();
}

void Game::respawn() {
    player_.spawn(level_.room.spawnX(), level_.room.spawnFeetY(), profile_.maxHp());
    shots_.clear();
    enemyShots_.clear();
    healItems_.clear();
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
                if (hasNextLevel()) {
                    enterLevel(levelIndex_ + 1);
                } else {
                    mode_ = GameMode::Finished;
                }
            }
            break;
        case GameMode::Finished:
            break;
    }
}

void Game::stepPlaying(const Input& in) {
    messageTimer_ = std::max(messageTimer_ - 1, 0);
    upgradeTimer_ = std::max(upgradeTimer_ - 1, 0);

    player_.update(in, level_.room, shots_);
    updateShots(shots_, level_.room);
    hazards_.update(level_.room, player_.bounds(), player_.grounded());
    updateEnemies(enemies_, enemyShots_, level_.room, player_.bounds(), strength_);
    updateEnemyShots(enemyShots_, level_.room);
    updateHealItems(healItems_, level_.room);
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
    collectItems();
    tryOpenDoor(in);
}

void Game::dropHealItem(const Enemy& enemy) {
    const std::optional<HealSize> size = rollDrop(strength_, random_);
    if (size) {
        healItems_.push_back(makeHealItem(*size, centerX(enemy.box), enemy.box.y + enemy.box.h / 2.0f));
    }
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
            dropHealItem(*hit);
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

void Game::collectItems() {
    const Box playerBox = player_.bounds();
    for (ToolPickup& tool : level_.tools) {
        if (!tool.collected && boxesOverlap(playerBox, tool.box)) {
            tool.collected = true;
        }
    }

    for (const HpUpgrade& upgrade : level_.hpUpgrades) {
        if (boxesOverlap(playerBox, upgrade.box) && profile_.addUpgrade(upgrade.id)) {
            player_.setMaxHp(profile_.maxHp());
            upgradeTimer_ = kMessageFrames;
        }
    }

    // Com a vida cheia o item fica no chao para quando precisar
    healItems_.erase(std::remove_if(healItems_.begin(), healItems_.end(),
                                    [this, &playerBox](const HealItem& item) {
                                        if (player_.hp() >= player_.maxHp() || !boxesOverlap(playerBox, item.box)) {
                                            return false;
                                        }
                                        player_.heal(healAmount(item.size));
                                        return true;
                                    }),
                     healItems_.end());
}

void Game::tryOpenDoor(const Input& in) {
    if (!in.upPressed || !boxesOverlap(player_.bounds(), level_.door)) {
        return;
    }
    if (toolsMissing() > 0) {
        messageTimer_ = kMessageFrames;
        return;
    }
    puzzle_ = WirePuzzle(level_.puzzle, static_cast<unsigned>(frame_));
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
    if (puzzle_.status() == WirePuzzle::Status::Solved) {
        doorOpen_ = true;
        mode_ = GameMode::Complete;
    }
}
