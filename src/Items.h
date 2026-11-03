#pragma once

#include <optional>
#include <random>
#include <vector>

#include "Room.h"

enum class HealSize { Small, Medium, Large };

struct HealItem {
    HealSize size;
    Box box;
    float vy;
    int life;  // passos restantes
};

int healAmount(HealSize size);
// No maximo um item por inimigo, sorteado do maior para o menor.
std::optional<HealSize> rollDrop(int strength, std::mt19937& random);
HealItem makeHealItem(HealSize size, float centerX, float centerY);
void updateHealItems(std::vector<HealItem>& items, const Room& room);
