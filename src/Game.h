#pragma once

#include <random>
#include <vector>

#include "Enemy.h"
#include "Hazards.h"
#include "Input.h"
#include "Items.h"
#include "LevelLoader.h"
#include "Player.h"
#include "Profile.h"
#include "Room.h"
#include "WirePuzzle.h"

// Finished: a fase acabou e o jogador confirmou na tela de bloco concluido.
enum class GameMode { Playing, Puzzle, Complete, Finished };

class Game {
public:
    // A morte dura kDeathFrames passos: o personagem pisca parado e, em
    // kDeathBurstFrame, explode em catraquinhas antes de renascer.
    static constexpr int kDeathFrames = 90;
    static constexpr int kDeathBurstFrame = 15;

    explicit Game(LevelData level, Profile profile = Profile(), unsigned seed = 1);
    // Comeca no bloco startLevel e grava no perfil cada bloco em que entra.
    Game(std::vector<LevelData> levels, Profile profile, unsigned seed, size_t startLevel = 0);

    void step(const Input& in);
    // Volta ao inicio do bloco mantendo as ferramentas ja coletadas.
    void respawn();
    void setStrength(int strength);

    GameMode mode() const { return mode_; }
    const Room& room() const { return level_.room; }
    const Player& player() const { return player_; }
    const std::vector<Shot>& shots() const { return shots_; }
    const std::vector<Enemy>& enemies() const { return enemies_; }
    const std::vector<EnemyShot>& enemyShots() const { return enemyShots_; }
    const std::vector<HealItem>& healItems() const { return healItems_; }
    const std::vector<ToolPickup>& tools() const { return level_.tools; }
    const std::vector<HpUpgrade>& hpUpgrades() const { return level_.hpUpgrades; }
    const Profile& profile() const { return profile_; }
    const Hazards& hazards() const { return hazards_; }
    const WirePuzzle& puzzle() const { return puzzle_; }
    const Box& door() const { return level_.door; }
    bool doorOpen() const { return doorOpen_; }
    bool hasNextLevel() const { return levelIndex_ + 1 < levels_.size(); }
    size_t levelIndex() const { return levelIndex_; }
    int strength() const { return strength_; }
    int toolsMissing() const;
    bool showingToolsMessage() const { return messageTimer_ > 0; }
    bool showingUpgradeMessage() const { return upgradeTimer_ > 0; }
    int frame() const { return frame_; }
    // Passos desde a morte; 0 com o jogador vivo.
    int deathFrames() const { return deathTimer_; }

private:
    void enterLevel(size_t index);
    void stepPlaying(const Input& in);
    void stepPuzzle(const Input& in);
    void resolveCombat();
    void dropHealItem(const Enemy& enemy);
    void collectItems();
    void tryOpenDoor(const Input& in);

    std::vector<LevelData> levels_;
    size_t levelIndex_ = 0;
    LevelData level_;
    Hazards hazards_;
    Profile profile_;
    std::mt19937 random_;
    Player player_;
    std::vector<Shot> shots_;
    std::vector<Enemy> enemies_;
    std::vector<EnemyShot> enemyShots_;
    std::vector<HealItem> healItems_;
    WirePuzzle puzzle_;
    GameMode mode_ = GameMode::Playing;
    int strength_ = 1;
    int frame_ = 0;
    int deathTimer_ = 0;
    int messageTimer_ = 0;
    int upgradeTimer_ = 0;
    bool doorOpen_ = false;
};
