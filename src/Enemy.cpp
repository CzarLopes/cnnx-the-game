#include "Enemy.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kCrawlerSpeed = 0.75f;
constexpr float kCrawlerFallSpeed = 2.0f;

constexpr float kFlyerSpeed = 0.6f;
constexpr float kFlyerSight = 96.0f;
constexpr float kFlyerBobHeight = 8.0f;
constexpr float kFlyerBobRate = 0.06f;

constexpr int kShooterCooldown = 120;
constexpr int kShooterFirstShot = 30;
constexpr float kShooterRangeX = 160.0f;
constexpr float kShooterRangeY = 64.0f;
constexpr float kProjectileSpeed = 2.0f;
constexpr float kProjectileSize = 6.0f;
constexpr int kProjectileLife = 150;

constexpr float kGrowthPerStrength = 2.0f;

float centerX(const Box& box) {
    return box.x + box.w / 2.0f;
}

float centerY(const Box& box) {
    return box.y + box.h / 2.0f;
}

void updateCrawler(Enemy& enemy, const Room& room, float speed) {
    Box below = enemy.box;
    below.y += 1.0f;
    if (!room.overlapsSolid(below)) {
        enemy.box.y += kCrawlerFallSpeed;
        if (room.overlapsSolid(enemy.box)) {
            const float feet = enemy.box.y + enemy.box.h;
            enemy.box.y = std::floor(feet / kTileSize) * kTileSize - enemy.box.h;
        }
        return;
    }

    Box next = enemy.box;
    next.x += static_cast<float>(enemy.dir) * speed;
    const float frontX = enemy.dir > 0 ? next.x + next.w : next.x - 1.0f;
    const Box frontFoot = {frontX, next.y + next.h, 1.0f, 1.0f};
    if (room.overlapsSolid(next) || !room.overlapsSolid(frontFoot)) {
        enemy.dir = -enemy.dir;
    } else {
        enemy.box = next;
    }
}

void updateFlyer(Enemy& enemy, const Room& room, const Box& target, float speed) {
    ++enemy.timer;
    const float dx = centerX(target) - centerX(enemy.box);
    const float dy = centerY(target) - centerY(enemy.box);
    const bool chasing = std::fabs(dx) < kFlyerSight && std::fabs(dy) < kFlyerSight;

    const float bob = std::sin(static_cast<float>(enemy.timer) * kFlyerBobRate) * kFlyerBobHeight;
    const float goalX = chasing ? centerX(target) : enemy.homeX;
    const float goalY = chasing ? centerY(target) : enemy.homeY + bob;

    Box next = enemy.box;
    next.x += std::clamp(goalX - centerX(enemy.box), -speed, speed);
    if (!room.overlapsSolid(next)) {
        enemy.box = next;
    }
    next = enemy.box;
    next.y += std::clamp(goalY - centerY(enemy.box), -speed, speed);
    if (!room.overlapsSolid(next)) {
        enemy.box = next;
    }
    if (dx != 0.0f) {
        enemy.dir = dx > 0.0f ? 1 : -1;
    }
}

void updateShooter(Enemy& enemy, std::vector<EnemyShot>& shots, const Box& target, const ForceStats& stats) {
    const float dx = centerX(target) - centerX(enemy.box);
    const float dy = centerY(target) - centerY(enemy.box);
    enemy.dir = dx >= 0.0f ? 1 : -1;

    if (enemy.timer > 0) {
        --enemy.timer;
        return;
    }
    if (std::fabs(dx) > kShooterRangeX || std::fabs(dy) > kShooterRangeY) {
        return;
    }
    enemy.timer = static_cast<int>(static_cast<float>(kShooterCooldown) / stats.speedScale);
    const float muzzleX = enemy.dir > 0 ? enemy.box.x + enemy.box.w : enemy.box.x - kProjectileSize;
    shots.push_back({{muzzleX, enemy.box.y + 4.0f, kProjectileSize, kProjectileSize},
                     static_cast<float>(enemy.dir) * kProjectileSpeed * stats.speedScale,
                     stats.projectileDamage,
                     kProjectileLife});
}

}  // namespace

const ForceStats& forceStats(int strength) {
    static constexpr ForceStats kStats[kMaxStrength] = {
        {2, 2, 2, 1.0f},
        {4, 3, 3, 1.15f},
        {7, 4, 4, 1.3f},
    };
    return kStats[std::clamp(strength, kMinStrength, kMaxStrength) - 1];
}

Enemy makeEnemy(const EnemySpawn& spawn, int strength) {
    const float growth = static_cast<float>(strength - 1) * kGrowthPerStrength;
    float w = 14.0f + growth;
    float h = 12.0f + growth;
    int timer = 0;
    if (spawn.kind == EnemyKind::Crawler) {
        h = 16.0f + growth;  // Alto o bastante para o tiro em pe acertar
    } else if (spawn.kind == EnemyKind::Shooter) {
        w = 16.0f + growth;
        h = 20.0f + growth;
        timer = kShooterFirstShot;
    }

    const float x = spawn.centerX - w / 2.0f;
    const float y = spawn.kind == EnemyKind::Flyer ? spawn.baseY - h / 2.0f : spawn.baseY - h;
    return {spawn.kind, {x, y, w, h}, forceStats(strength).hp, -1, timer, spawn.centerX, spawn.baseY};
}

void updateEnemies(std::vector<Enemy>& enemies, std::vector<EnemyShot>& shots, const Room& room,
                   const Box& target, int strength) {
    const ForceStats& stats = forceStats(strength);
    for (Enemy& enemy : enemies) {
        switch (enemy.kind) {
            case EnemyKind::Crawler:
                updateCrawler(enemy, room, kCrawlerSpeed * stats.speedScale);
                break;
            case EnemyKind::Flyer:
                updateFlyer(enemy, room, target, kFlyerSpeed * stats.speedScale);
                break;
            case EnemyKind::Shooter:
                updateShooter(enemy, shots, target, stats);
                break;
        }
    }
}

void updateEnemyShots(std::vector<EnemyShot>& shots, const Room& room) {
    for (EnemyShot& shot : shots) {
        shot.box.x += shot.vx;
        --shot.life;
    }
    shots.erase(std::remove_if(shots.begin(), shots.end(),
                               [&room](const EnemyShot& shot) {
                                   return shot.life <= 0 || room.overlapsSolid(shot.box);
                               }),
                shots.end());
}
