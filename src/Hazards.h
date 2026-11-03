#pragma once

#include <vector>

#include "Room.h"

struct CrumbleBlock {
    enum class Phase { Stable, Shaking, Fallen };

    int tileX;
    int tileY;
    Phase phase;
    int timer;
    int fallFrames = 0;  // passos desde que soltou, para desenhar a queda
};

// Objeto de montagem suspenso que despenca quando o jogador se aproxima.
struct FallingObject {
    enum class Phase { Hanging, Warning, Falling, Gone };

    Box home;
    Box box;
    Phase phase;
    int timer;
    float vy;
};

class Hazards {
public:
    Hazards(const Room& room, std::vector<Box> darkZones, const std::vector<Box>& fallingObjects = {});

    void reset(Room& room);
    void update(Room& room, const Box& player, bool playerGrounded);

    bool shockOn() const;
    // Dano do perigo que encosta no box, tile ou objeto caindo; 0 se nenhum.
    int damageAt(const Room& room, const Box& box) const;
    // 0 = visivel, 1 = totalmente escuro, para um ponto da sala.
    float darknessAt(float x, float y, float playerX, float playerY) const;

    const std::vector<CrumbleBlock>& crumbles() const { return crumbles_; }
    const std::vector<FallingObject>& fallingObjects() const { return fallingObjects_; }
    const std::vector<Box>& darkZones() const { return darkZones_; }

private:
    void updateFallingObject(FallingObject& object, const Room& room, const Box& player);

    std::vector<CrumbleBlock> crumbles_;
    std::vector<FallingObject> fallingObjects_;
    std::vector<Box> darkZones_;
    int clock_ = 0;
};
