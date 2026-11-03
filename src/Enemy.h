#pragma once

#include <vector>

#include "Room.h"

// Todo bloco tem os tres voadores: o passivo so atrapalha o caminho, o
// perseguidor vem para cima e o atirador mira no jogador.
enum class EnemyKind { Crawler, Shooter, FlyerPassive, FlyerChaser, FlyerShooter };

constexpr bool isFlyer(EnemyKind kind) {
    return kind == EnemyKind::FlyerPassive || kind == EnemyKind::FlyerChaser || kind == EnemyKind::FlyerShooter;
}

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
    int age = 0;
};

struct EnemyShot {
    Box box;
    float vx;
    float vy;
    int damage;
    int life;
};

Enemy makeEnemy(const EnemySpawn& spawn, int strength);
void updateEnemies(std::vector<Enemy>& enemies, std::vector<EnemyShot>& shots, const Room& room,
                   const Box& target, int strength);
void updateEnemyShots(std::vector<EnemyShot>& shots, const Room& room);
