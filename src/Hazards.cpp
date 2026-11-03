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
constexpr float kLightRadius = 40.0f;
constexpr float kLightFade = 32.0f;

Box tileBox(const CrumbleBlock& block) {
    return {static_cast<float>(block.tileX * kTileSize), static_cast<float>(block.tileY * kTileSize),
            static_cast<float>(kTileSize), static_cast<float>(kTileSize)};
}

}  // namespace

Hazards::Hazards(const Room& room, std::vector<Box> darkZones) : darkZones_(std::move(darkZones)) {
    for (int y = 0; y < room.heightTiles(); ++y) {
        for (int x = 0; x < room.widthTiles(); ++x) {
            if (room.tileAt(x, y) == kTileCrumble) {
                crumbles_.push_back({x, y, CrumbleBlock::Phase::Stable, 0});
            }
        }
    }
}

void Hazards::reset(Room& room) {
    for (CrumbleBlock& block : crumbles_) {
        block.phase = CrumbleBlock::Phase::Stable;
        block.timer = 0;
        room.setTile(block.tileX, block.tileY, kTileCrumble);
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
                    room.setTile(block.tileX, block.tileY, kTileEmpty);
                }
                break;
            case CrumbleBlock::Phase::Fallen:
                // Espera o jogador sair, para o bloco nao voltar em cima dele
                if (--block.timer <= 0 && !boxesOverlap(player, tileBox(block))) {
                    block.phase = CrumbleBlock::Phase::Stable;
                    room.setTile(block.tileX, block.tileY, kTileCrumble);
                }
                break;
        }
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
    return 0;
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
