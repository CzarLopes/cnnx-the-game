#include "Hazards.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

constexpr int kSpikeDamage = 4;
constexpr int kShockDamage = 3;
constexpr int kShockPeriod = 150;
constexpr int kShockOnFrames = 75;
constexpr int kCrumbleDelay = 30;
constexpr int kCrumbleReturn = 180;
constexpr int kFallingDamage = 3;
constexpr float kFallingTriggerX = 64.0f;
constexpr int kFallingWarning = 20;
constexpr float kFallingGravity = 0.25f;
constexpr float kFallingMaxSpeed = 6.0f;
constexpr int kFallingReturn = 240;
constexpr float kLightRadius = 40.0f;
constexpr float kLightFade = 32.0f;

Box tileBox(const CrumbleBlock& block) {
    return {static_cast<float>(block.tileX * kTileSize), static_cast<float>(block.tileY * kTileSize),
            static_cast<float>(kTileSize), static_cast<float>(kTileSize)};
}

}  // namespace

Hazards::Hazards(const Room& room, std::vector<Box> darkZones, const std::vector<Box>& fallingObjects)
    : darkZones_(std::move(darkZones)) {
    for (int y = 0; y < room.heightTiles(); ++y) {
        for (int x = 0; x < room.widthTiles(); ++x) {
            if (room.tileAt(x, y) == kTileCrumble) {
                crumbles_.push_back({x, y, CrumbleBlock::Phase::Stable, 0});
            }
        }
    }
    for (const Box& box : fallingObjects) {
        fallingObjects_.push_back({box, box, FallingObject::Phase::Hanging, 0, 0.0f});
    }
}

void Hazards::reset(Room& room) {
    for (CrumbleBlock& block : crumbles_) {
        block.phase = CrumbleBlock::Phase::Stable;
        block.timer = 0;
        block.fallFrames = 0;
        room.setTile(block.tileX, block.tileY, kTileCrumble);
    }
    for (FallingObject& object : fallingObjects_) {
        object.box = object.home;
        object.phase = FallingObject::Phase::Hanging;
        object.timer = 0;
        object.vy = 0.0f;
    }
}

void Hazards::updateFallingObject(FallingObject& object, const Room& room, const Box& player) {
    switch (object.phase) {
        case FallingObject::Phase::Hanging: {
            const float dx = (player.x + player.w / 2.0f) - (object.box.x + object.box.w / 2.0f);
            if (std::fabs(dx) <= kFallingTriggerX && player.y > object.box.y + object.box.h) {
                object.phase = FallingObject::Phase::Warning;
                object.timer = kFallingWarning;
            }
            break;
        }
        case FallingObject::Phase::Warning:
            if (--object.timer <= 0) {
                object.phase = FallingObject::Phase::Falling;
            }
            break;
        case FallingObject::Phase::Falling:
            object.vy = std::min(object.vy + kFallingGravity, kFallingMaxSpeed);
            object.box.y += object.vy;
            if (room.overlapsSolid(object.box)) {
                object.phase = FallingObject::Phase::Gone;
                object.timer = kFallingReturn;
                object.box = object.home;
                object.vy = 0.0f;
            }
            break;
        case FallingObject::Phase::Gone:
            if (--object.timer <= 0) {
                object.phase = FallingObject::Phase::Hanging;
            }
            break;
    }
}

void Hazards::update(Room& room, const Box& player, bool playerGrounded) {
    ++clock_;
    const Box feet = {player.x, player.y + player.h, player.w, 1.0f};

    for (CrumbleBlock& block : crumbles_) {
        switch (block.phase) {
            case CrumbleBlock::Phase::Stable:
                if (playerGrounded && boxesOverlap(feet, tileBox(block))) {
                    block.phase = CrumbleBlock::Phase::Shaking;
                    block.timer = kCrumbleDelay;
                }
                break;
            case CrumbleBlock::Phase::Shaking:
                if (--block.timer <= 0) {
                    block.phase = CrumbleBlock::Phase::Fallen;
                    block.timer = kCrumbleReturn;
                    block.fallFrames = 0;
                    room.setTile(block.tileX, block.tileY, kTileEmpty);
                }
                break;
            case CrumbleBlock::Phase::Fallen:
                ++block.fallFrames;
                // Espera o jogador sair, para o bloco nao voltar em cima dele
                if (--block.timer <= 0 && !boxesOverlap(player, tileBox(block))) {
                    block.phase = CrumbleBlock::Phase::Stable;
                    room.setTile(block.tileX, block.tileY, kTileCrumble);
                }
                break;
        }
    }

    for (FallingObject& object : fallingObjects_) {
        updateFallingObject(object, room, player);
    }
}

bool Hazards::shockOn() const {
    return clock_ % kShockPeriod < kShockOnFrames;
}

int Hazards::damageAt(const Room& room, const Box& box) const {
    const Box probe = {box.x, box.y - 1.0f, box.w, box.h + 2.0f};
    if (room.overlapsTile(probe, kTileSpike)) {
        return kSpikeDamage;
    }
    if (shockOn() && room.overlapsTile(probe, kTileShock)) {
        return kShockDamage;
    }
    const bool hitByObject = std::any_of(fallingObjects_.begin(), fallingObjects_.end(), [&box](const FallingObject& object) {
        return object.phase == FallingObject::Phase::Falling && boxesOverlap(box, object.box);
    });
    return hitByObject ? kFallingDamage : 0;
}

float Hazards::darknessAt(float x, float y, float playerX, float playerY) const {
    const bool dark = std::any_of(darkZones_.begin(), darkZones_.end(), [x, y](const Box& zone) {
        return x >= zone.x && x < zone.x + zone.w && y >= zone.y && y < zone.y + zone.h;
    });
    if (!dark) {
        return 0.0f;
    }
    const float distance = std::hypot(x - playerX, y - playerY);
    return std::clamp((distance - kLightRadius) / kLightFade, 0.0f, 1.0f);
}
