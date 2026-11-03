#pragma once

#include <vector>

#include "Room.h"

struct CrumbleBlock {
    enum class Phase { Stable, Shaking, Fallen };

    int tileX;
    int tileY;
    Phase phase;
    int timer;
};

class Hazards {
public:
    Hazards(const Room& room, std::vector<Box> darkZones);

    void reset(Room& room);
    void update(Room& room, const Box& player, bool playerGrounded);

    bool shockOn() const;
    // Dano do tile perigoso que encosta no box por cima ou por baixo; 0 se nenhum.
    int damageAt(const Room& room, const Box& box) const;
    // 0 = visivel, 1 = totalmente escuro, para um ponto da sala.
    float darknessAt(float x, float y, float playerX, float playerY) const;

    const std::vector<CrumbleBlock>& crumbles() const { return crumbles_; }
    const std::vector<Box>& darkZones() const { return darkZones_; }

private:
    std::vector<CrumbleBlock> crumbles_;
    std::vector<Box> darkZones_;
    int clock_ = 0;
};
