#include "Items.h"

#include <algorithm>
#include <cmath>

#include "Enemy.h"

namespace {

constexpr float kLargeChance = 0.2f;
constexpr float kMediumChance = 0.4f;
constexpr float kSmallChance = 0.7f;

constexpr float kItemGravity = 0.25f;
constexpr float kItemMaxFall = 4.0f;
constexpr int kItemLife = 480;

}  // namespace

int healAmount(HealSize size) {
    switch (size) {
        case HealSize::Small: return 2;
        case HealSize::Medium: return 5;
        case HealSize::Large: return 10;
    }
    return 0;
}

std::optional<HealSize> rollDrop(int strength, std::mt19937& random) {
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    if (strength >= kMaxStrength && chance(random) < kLargeChance) {
        return HealSize::Large;
    }
    if (chance(random) < kMediumChance) {
        return HealSize::Medium;
    }
    if (chance(random) < kSmallChance) {
        return HealSize::Small;
    }
    return std::nullopt;
}

HealItem makeHealItem(HealSize size, float centerX, float centerY) {
    const float side = size == HealSize::Small ? 6.0f : size == HealSize::Medium ? 8.0f : 12.0f;
    return {size, {centerX - side / 2.0f, centerY - side / 2.0f, side, side}, 0.0f, kItemLife};
}

void updateHealItems(std::vector<HealItem>& items, const Room& room) {
    for (HealItem& item : items) {
        --item.life;
        item.vy = std::min(item.vy + kItemGravity, kItemMaxFall);
        Box next = item.box;
        next.y += item.vy;
        if (room.overlapsSolid(next)) {
            item.box.y = std::floor((next.y + next.h) / kTileSize) * kTileSize - next.h;
            item.vy = 0.0f;
        } else {
            item.box = next;
        }
    }
    items.erase(std::remove_if(items.begin(), items.end(), [](const HealItem& item) { return item.life <= 0; }),
                items.end());
}
