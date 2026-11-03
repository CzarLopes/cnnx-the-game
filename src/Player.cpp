#include "Player.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kWalkSpeed = 1.5f;
constexpr float kDashSpeed = 3.5f;
constexpr int kDashFrames = 32;
constexpr int kAirDashFrames = 20;

constexpr float kGravity = 0.25f;
constexpr float kMaxFallSpeed = 5.75f;
constexpr float kJumpSpeed = 5.0f;
constexpr int kJumpBufferFrames = 4;
constexpr int kCoyoteFrames = 4;

constexpr float kWallSlideSpeed = 2.0f;
constexpr float kWallKickReach = 3.0f;
constexpr float kWallKickSpeed = 1.5f;
constexpr int kWallKickFrames = 8;

constexpr float kShotSpeed = 6.0f;
constexpr int kShotLife = 60;
constexpr int kMaxNormalShots = 3;
constexpr float kMuzzleOffsetY = 12.0f;

constexpr float kClimbSpeed = 1.5f;

constexpr int kInvulnFrames = 60;
constexpr int kHurtFrames = 18;
constexpr float kKnockbackSpeedX = 1.0f;
constexpr float kKnockbackSpeedY = 2.5f;

}  // namespace

const char* playerStateName(PlayerState state) {
    switch (state) {
        case PlayerState::Idle: return "Idle";
        case PlayerState::Run: return "Run";
        case PlayerState::Jump: return "Jump";
        case PlayerState::Fall: return "Fall";
        case PlayerState::Dash: return "Dash";
        case PlayerState::AirDash: return "AirDash";
        case PlayerState::WallSlide: return "WallSlide";
        case PlayerState::WallKick: return "WallKick";
        case PlayerState::Climb: return "Climb";
        case PlayerState::Hurt: return "Hurt";
        case PlayerState::Dead: return "Dead";
    }
    return "?";
}

Box shotBox(const Shot& shot) {
    static constexpr float kSizes[3][2] = {{6.0f, 4.0f}, {10.0f, 8.0f}, {16.0f, 12.0f}};
    const float w = kSizes[shot.level][0];
    const float h = kSizes[shot.level][1];
    return {shot.x - w / 2.0f, shot.y - h / 2.0f, w, h};
}

int shotDamage(const Shot& shot) {
    static constexpr int kDamage[3] = {1, 2, 4};
    return kDamage[shot.level];
}

void updateShots(std::vector<Shot>& shots, const Room& room) {
    for (Shot& shot : shots) {
        shot.x += shot.vx;
        --shot.life;
    }
    shots.erase(std::remove_if(shots.begin(), shots.end(),
                               [&room](const Shot& shot) {
                                   return shot.life <= 0 || room.overlapsSolid(shotBox(shot));
                               }),
                shots.end());
}

void Player::spawn(float centerX, float feetY, int maxHp) {
    *this = Player{};
    maxHp_ = maxHp;
    hp_ = maxHp;
    x_ = centerX - kWidth / 2.0f;
    y_ = feetY - kHeight;
    grounded_ = true;
}

void Player::update(const Input& in, const Room& room, std::vector<Shot>& shots) {
    invulnTimer_ = std::max(invulnTimer_ - 1, 0);
    if (state_ == PlayerState::Dead) {
        return;
    }
    if (state_ == PlayerState::Hurt) {
        updateHurt(room);
        return;
    }

    const int moveDir = static_cast<int>(in.right) - static_cast<int>(in.left);

    jumpBuffer_ = in.jumpPressed ? kJumpBufferFrames : std::max(jumpBuffer_ - 1, 0);
    coyote_ = grounded_ ? kCoyoteFrames : std::max(coyote_ - 1, 0);

    if (state_ == PlayerState::Climb || tryStartClimb(in, room)) {
        updateClimb(in, moveDir, room);
        updateBuster(in, shots);
        return;
    }

    // Na parede o personagem fica virado para fora, para atirar para longe dela.
    // No dash a direcao fica travada; apertar para o lado oposto cancela.
    if (moveDir != 0 && state_ != PlayerState::WallSlide && state_ != PlayerState::WallKick &&
        state_ != PlayerState::Dash && state_ != PlayerState::AirDash) {
        facing_ = moveDir;
    }

    // Antes do pulo: dash e pulo no mesmo passo viram pulo com dash, nao dash aereo
    if (in.dashPressed) {
        if (grounded_ && state_ != PlayerState::Dash) {
            startDash();
        } else if (airDashAvailable_ && (state_ == PlayerState::Jump || state_ == PlayerState::Fall)) {
            startAirDash();
        }
    }

    if (jumpBuffer_ > 0) {
        const int nearWall = wallDirection(room, kWallKickReach);
        if (coyote_ > 0) {
            startJump();
        } else if (nearWall != 0) {
            startWallKick(nearWall, in.dashHeld);
        }
    }

    switch (state_) {
        case PlayerState::Dash:
            --dashTimer_;
            if (dashTimer_ <= 0 || !in.dashHeld || moveDir == -facing_) {
                state_ = PlayerState::Idle;
                vx_ = static_cast<float>(moveDir) * kWalkSpeed;
            } else {
                vx_ = static_cast<float>(facing_) * kDashSpeed;
            }
            break;
        case PlayerState::AirDash:
            --dashTimer_;
            if (dashTimer_ <= 0 || !in.dashHeld || moveDir == -facing_) {
                state_ = PlayerState::Fall;
                vx_ = static_cast<float>(moveDir) * (dashMomentum_ ? kDashSpeed : kWalkSpeed);
            } else {
                vx_ = static_cast<float>(facing_) * kDashSpeed;
            }
            break;
        case PlayerState::WallKick:
            --kickTimer_;
            vx_ = static_cast<float>(kickDir_) * (dashMomentum_ ? kDashSpeed : kWallKickSpeed);
            if (kickTimer_ <= 0) {
                state_ = PlayerState::Jump;
            }
            break;
        default:
            vx_ = static_cast<float>(moveDir) * (dashMomentum_ ? kDashSpeed : kWalkSpeed);
            // Soltar o botao corta a subida: pulo curto ou longo
            if (!in.jumpHeld && vy_ < 0.0f) {
                vy_ = 0.0f;
            }
            break;
    }

    if (state_ == PlayerState::WallSlide) {
        vy_ = kWallSlideSpeed;
    } else if (state_ == PlayerState::AirDash) {
        vy_ = 0.0f;
    } else {
        vy_ = std::min(vy_ + kGravity, kMaxFallSpeed);
    }

    if (moveX(vx_, room)) {
        vx_ = 0.0f;
        if (state_ == PlayerState::Dash) {
            state_ = PlayerState::Idle;
        } else if (state_ == PlayerState::AirDash) {
            state_ = PlayerState::Fall;
        }
    }

    const bool hitVertical = moveY(vy_, room);
    grounded_ = hitVertical && vy_ > 0.0f;
    if (hitVertical) {
        vy_ = 0.0f;
    }

    if (grounded_) {
        dashMomentum_ = false;
        airDashAvailable_ = true;
        if (state_ != PlayerState::Dash) {
            state_ = (vx_ != 0.0f) ? PlayerState::Run : PlayerState::Idle;
        }
    } else if (state_ == PlayerState::Dash) {
        // Saiu de uma beirada durante o dash: continua rapido no ar
        dashMomentum_ = true;
        state_ = PlayerState::Fall;
    } else if (state_ != PlayerState::WallKick && state_ != PlayerState::AirDash) {
        if (vy_ >= 0.0f && moveDir != 0 && wallDirection(room, 1.0f) == moveDir) {
            state_ = PlayerState::WallSlide;
            facing_ = -moveDir;
            dashMomentum_ = false;
            airDashAvailable_ = true;
        } else {
            state_ = (vy_ < 0.0f) ? PlayerState::Jump : PlayerState::Fall;
        }
    }

    updateBuster(in, shots);
}

bool Player::takeDamage(int amount, int knockDir) {
    if (invulnTimer_ > 0 || state_ == PlayerState::Dead) {
        return false;
    }
    hp_ = std::max(hp_ - amount, 0);
    dashMomentum_ = false;
    grounded_ = false;
    if (hp_ == 0) {
        state_ = PlayerState::Dead;
        vx_ = 0.0f;
        vy_ = 0.0f;
        return true;
    }
    state_ = PlayerState::Hurt;
    hurtTimer_ = kHurtFrames;
    invulnTimer_ = kInvulnFrames;
    vx_ = static_cast<float>(knockDir) * kKnockbackSpeedX;
    vy_ = -kKnockbackSpeedY;
    return true;
}

void Player::heal(int amount) {
    hp_ = std::min(hp_ + amount, maxHp_);
}

void Player::setMaxHp(int maxHp) {
    hp_ += std::max(maxHp - maxHp_, 0);
    maxHp_ = maxHp;
}

void Player::updateHurt(const Room& room) {
    vy_ = std::min(vy_ + kGravity, kMaxFallSpeed);
    if (moveX(vx_, room)) {
        vx_ = 0.0f;
    }
    const bool hitVertical = moveY(vy_, room);
    grounded_ = hitVertical && vy_ > 0.0f;
    if (hitVertical) {
        vy_ = 0.0f;
    }
    if (--hurtTimer_ <= 0) {
        state_ = grounded_ ? PlayerState::Idle : PlayerState::Fall;
    }
}

bool Player::tryStartClimb(const Input& in, const Room& room) {
    const float centerX = x_ + kWidth / 2.0f;
    const int column = static_cast<int>(std::floor(centerX / kTileSize));
    const int feetRow = static_cast<int>(std::floor((y_ + kHeight) / kTileSize));

    bool grab = false;
    if (in.up) {
        grab = room.overlapsTile({centerX, y_, 1.0f, kHeight}, kTileLadder);
    } else if (in.down && grounded_ && room.isLadderTop(column, feetRow)) {
        grab = true;
        y_ += 2.0f;  // Passa do topo, que e plataforma de mao unica
    }
    if (!grab) {
        return false;
    }

    state_ = PlayerState::Climb;
    x_ = (static_cast<float>(column) + 0.5f) * kTileSize - kWidth / 2.0f;
    vx_ = 0.0f;
    vy_ = 0.0f;
    grounded_ = false;
    dashMomentum_ = false;
    airDashAvailable_ = true;
    return true;
}

void Player::updateClimb(const Input& in, int moveDir, const Room& room) {
    if (moveDir != 0) {
        facing_ = moveDir;
    }
    if (in.jumpPressed) {
        state_ = PlayerState::Fall;
        jumpBuffer_ = 0;
        return;
    }

    vy_ = (static_cast<float>(in.down) - static_cast<float>(in.up)) * kClimbSpeed;
    const bool movingDown = vy_ > 0.0f;
    if (moveY(vy_, room) && movingDown) {
        state_ = PlayerState::Idle;
        grounded_ = true;
        vy_ = 0.0f;
        return;
    }

    const float centerX = x_ + kWidth / 2.0f;
    const int column = static_cast<int>(std::floor(centerX / kTileSize));
    const int feetRow = static_cast<int>(std::floor((y_ + kHeight - 1.0f) / kTileSize));
    if (room.tileAt(column, feetRow) != kTileLadder && room.isLadderTop(column, feetRow + 1)) {
        y_ = static_cast<float>((feetRow + 1) * kTileSize) - kHeight;
        state_ = PlayerState::Idle;
        grounded_ = true;
        vy_ = 0.0f;
    } else if (!room.overlapsTile({centerX, y_, 1.0f, kHeight}, kTileLadder)) {
        state_ = PlayerState::Fall;
    }
}

void Player::startJump() {
    if (state_ == PlayerState::Dash || state_ == PlayerState::AirDash) {
        dashMomentum_ = true;
    }
    vy_ = -kJumpSpeed;
    state_ = PlayerState::Jump;
    grounded_ = false;
    coyote_ = 0;
    jumpBuffer_ = 0;
}

void Player::startWallKick(int wallDir, bool dashHeld) {
    kickDir_ = -wallDir;
    facing_ = kickDir_;
    kickTimer_ = kWallKickFrames;
    dashMomentum_ = dashHeld;
    airDashAvailable_ = true;
    vy_ = -kJumpSpeed;
    state_ = PlayerState::WallKick;
    jumpBuffer_ = 0;
}

void Player::startDash() {
    state_ = PlayerState::Dash;
    dashTimer_ = kDashFrames;
}

void Player::startAirDash() {
    state_ = PlayerState::AirDash;
    dashTimer_ = kAirDashFrames;
    airDashAvailable_ = false;
    vy_ = 0.0f;
}

void Player::updateBuster(const Input& in, std::vector<Shot>& shots) {
    if (in.shootPressed) {
        fireShot(0, shots);
    }
    if (in.shootHeld) {
        charge_ = std::min(charge_ + 1, kChargeLevel2);
    } else {
        if (charge_ >= kChargeLevel1) {
            fireShot(charge_ >= kChargeLevel2 ? 2 : 1, shots);
        }
        charge_ = 0;
    }
}

void Player::fireShot(int level, std::vector<Shot>& shots) {
    if (level == 0) {
        const auto normalShots = std::count_if(shots.begin(), shots.end(),
                                               [](const Shot& shot) { return shot.level == 0; });
        if (normalShots >= kMaxNormalShots) {
            return;
        }
    }
    const float muzzleX = x_ + kWidth / 2.0f + static_cast<float>(facing_) * (kWidth / 2.0f + 4.0f);
    shots.push_back({muzzleX, y_ + kMuzzleOffsetY, static_cast<float>(facing_) * kShotSpeed, level, kShotLife});
}

int Player::wallDirection(const Room& room, float reach) const {
    if (room.overlapsSolid({x_ + reach, y_, kWidth, kHeight})) {
        return 1;
    }
    if (room.overlapsSolid({x_ - reach, y_, kWidth, kHeight})) {
        return -1;
    }
    return 0;
}

bool Player::moveX(float dx, const Room& room) {
    x_ += dx;
    if (!room.overlapsSolid(bounds())) {
        return false;
    }
    if (dx > 0.0f) {
        x_ = std::floor((x_ + kWidth) / kTileSize) * kTileSize - kWidth;
    } else {
        x_ = (std::floor(x_ / kTileSize) + 1.0f) * kTileSize;
    }
    return true;
}

bool Player::moveY(float dy, const Room& room) {
    const float previousFeet = y_ + kHeight;
    y_ += dy;
    if (!room.overlapsSolid(bounds())) {
        if (dy <= 0.0f || state_ == PlayerState::Climb) {
            return false;
        }
        const int feetRow = static_cast<int>(std::floor((y_ + kHeight) / kTileSize));
        const float rowTop = static_cast<float>(feetRow * kTileSize);
        if (previousFeet > rowTop || y_ + kHeight <= rowTop) {
            return false;
        }
        const int left = static_cast<int>(std::floor(x_ / kTileSize));
        const int right = static_cast<int>(std::ceil((x_ + kWidth) / kTileSize)) - 1;
        for (int column = left; column <= right; ++column) {
            if (room.isLadderTop(column, feetRow)) {
                y_ = rowTop - kHeight;
                return true;
            }
        }
        return false;
    }
    if (dy > 0.0f) {
        y_ = std::floor((y_ + kHeight) / kTileSize) * kTileSize - kHeight;
    } else {
        y_ = (std::floor(y_ / kTileSize) + 1.0f) * kTileSize;
    }
    return true;
}
