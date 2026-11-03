#pragma once

#include <string>
#include <vector>

#include "Enemy.h"
#include "Room.h"
#include "WirePuzzle.h"

struct ToolPickup {
    std::string kind;
    Box box;
    bool collected;
};

// Conquista escondida que aumenta a vida maxima de forma permanente.
struct HpUpgrade {
    std::string id;
    Box box;
};

struct LevelData {
    Room room;
    std::vector<EnemySpawn> enemies;
    std::vector<ToolPickup> tools;
    Box door;
    std::vector<Box> darkZones;
    int strength;
    std::vector<Box> fallingObjects = {};
    std::vector<HpUpgrade> hpUpgrades = {};
    PuzzleKind puzzle = PuzzleKind::Rj45;
};

// Lanca std::runtime_error com o motivo se o arquivo ou o nivel estiverem invalidos.
LevelData parseLevel(const std::string& ldtkJson, const std::string& levelId);
LevelData loadLevel(const std::string& ldtkPath, const std::string& levelId);
