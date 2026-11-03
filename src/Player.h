#pragma once

#include <vector>

#include "Input.h"
#include "Profile.h"
#include "Room.h"

enum class PlayerState { Idle, Run, Jump, Fall, Dash, AirDash, WallSlide, WallKick, Climb, Hurt, Dead };

const char* playerStateName(PlayerState state);

struct Shot {
    float x;  // centro
    float y;  // centro
    float vx;
    int level;  // 0 = normal, 1 = carga media, 2 = carga total
    int life;   // passos restantes
};

Box shotBox(const Shot& shot);
int shotDamage(const Shot& shot);
void updateShots(std::vector<Shot>& shots, const Room& room);

// Velocidades em pixels por passo; a simulacao roda a 60 passos por segundo.
class Player {
public:
    static constexpr float kWidth = 14.0f;
    static constexpr float kHeight = 28.0f;
    static constexpr int kChargeLevel1 = 30;
    static constexpr int kChargeLevel2 = 90;

    void spawn(float centerX, float feetY, int maxHp = Profile::kBaseMaxHp);
    void update(const Input& in, const Room& room, std::vector<Shot>& shots);
    // knockDir empurra para esse lado. Retorna false se o dano foi ignorado.
    bool takeDamage(int amount, int knockDir);
    void heal(int amount);
    // O ganho de vida maxima tambem entra na vida atual.
    void setMaxHp(int maxHp);

    Box bounds() const { return {x_, y_, kWidth, kHeight}; }
    PlayerState state() const { return state_; }
    int facing() const { return facing_; }
    int charge() const { return charge_; }
    bool grounded() const { return grounded_; }
    float vx() const { return vx_; }
    float vy() const { return vy_; }
    int hp() const { return hp_; }
    int maxHp() const { return maxHp_; }
    bool isDead() const { return state_ == PlayerState::Dead; }
    bool invulnerable() const { return invulnTimer_ > 0; }

private:
    bool tryStartClimb(const Input& in, const Room& room);
    void updateClimb(const Input& in, int moveDir, const Room& room);
    void updateHurt(const Room& room);
    void startJump();
    void startWallKick(int wallDir, bool dashHeld);
    void startDash();
    void startAirDash();
    void updateBuster(const Input& in, std::vector<Shot>& shots);
    void fireShot(int level, std::vector<Shot>& shots);
    int wallDirection(const Room& room, float reach) const;
    bool moveX(float dx, const Room& room);
    bool moveY(float dy, const Room& room);

    float x_ = 0.0f;  // canto superior esquerdo
    float y_ = 0.0f;
    float vx_ = 0.0f;
    float vy_ = 0.0f;
    int facing_ = 1;
    PlayerState state_ = PlayerState::Idle;
    bool grounded_ = false;
    bool dashMomentum_ = false;  // mantem a velocidade do dash no ar
    bool airDashAvailable_ = true;  // recarrega ao tocar o chao ou uma parede
    int dashTimer_ = 0;
    int kickTimer_ = 0;
    int kickDir_ = 0;
    int jumpBuffer_ = 0;
    int coyote_ = 0;
    int charge_ = 0;
    int maxHp_ = Profile::kBaseMaxHp;
    int hp_ = Profile::kBaseMaxHp;
    int invulnTimer_ = 0;
    int hurtTimer_ = 0;
};
