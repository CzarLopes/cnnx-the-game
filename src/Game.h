#pragma once

#include <vector>

#include "Enemy.h"
#include "Hazards.h"
#include "Input.h"
#include "LevelLoader.h"
#include "Player.h"
#include "PuzzleRj45.h"
#include "Room.h"

enum class GameMode { Playing, Puzzle, Complete };

class Game {
public:
    explicit Game(LevelData level);

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
    const std::vector<ToolPickup>& tools() const { return level_.tools; }
    const Hazards& hazards() const { return hazards_; }
    const PuzzleRj45& puzzle() const { return puzzle_; }
    const Box& door() const { return level_.door; }
    bool doorOpen() const { return doorOpen_; }
    int strength() const { return strength_; }
    int toolsMissing() const;
    bool showingToolsMessage() const { return messageTimer_ > 0; }
    int frame() const { return frame_; }

private:
    void stepPlaying(const Input& in);
    void stepPuzzle(const Input& in);
    void resolveCombat();
    void collectTools();
    void tryOpenDoor(const Input& in);
    void restartBlock();

    LevelData level_;
    Hazards hazards_;
    Player player_;
    std::vector<Shot> shots_;
    std::vector<Enemy> enemies_;
    std::vector<EnemyShot> enemyShots_;
    PuzzleRj45 puzzle_;
    GameMode mode_ = GameMode::Playing;
    int strength_;
    int frame_ = 0;
    int deathTimer_ = 0;
    int messageTimer_ = 0;
    bool doorOpen_ = false;
};
