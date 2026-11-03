#pragma once

#include <string>
#include <vector>

#include "Enemy.h"
#include "Room.h"

struct ToolPickup {
    std::string kind;
    Box box;
    bool collected;
};

struct LevelData {
    Room room;
    std::vector<EnemySpawn> enemies;
    std::vector<ToolPickup> tools;
    Box door;
    std::vector<Box> darkZones;
    int strength;
    std::vector<Box> fallingObjects = {};
};

// Lanca std::runtime_error com o motivo se o arquivo ou o nivel estiverem invalidos.
LevelData parseLevel(const std::string& ldtkJson, const std::string& levelId);
LevelData loadLevel(const std::string& ldtkPath, const std::string& levelId);
