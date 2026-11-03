#pragma once

#include <vector>

#include "Room.h"

enum class EnemyKind { Crawler, Flyer, Shooter };

struct ForceStats {
    int hp;
    int contactDamage;
    int projectileDamage;
    float speedScale;
};

constexpr int kMinStrength = 1;
constexpr int kMaxStrength = 3;

const ForceStats& forceStats(int strength);

struct EnemySpawn {
    EnemyKind kind;
    float centerX;
    float baseY;  // pes no rastejante e no lancador, centro no voador
};

struct Enemy {
    EnemyKind kind;
    Box box;
    int hp;
    int dir;
    int timer;
    float homeX;  // centro
    float homeY;
};

struct EnemyShot {
    Box box;
    float vx;
    int damage;
    int life;
};

Enemy makeEnemy(const EnemySpawn& spawn, int strength);
void updateEnemies(std::vector<Enemy>& enemies, std::vector<EnemyShot>& shots, const Room& room,
                   const Box& target, int strength);
void updateEnemyShots(std::vector<EnemyShot>& shots, const Room& room);
