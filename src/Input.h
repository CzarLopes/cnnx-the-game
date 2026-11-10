#pragma once

// Estado dos botoes em um passo da simulacao. Os campos "Pressed" valem true
// somente no passo em que o botao foi apertado.
struct Input {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool leftPressed = false;
    bool rightPressed = false;
    bool upPressed = false;
    bool downPressed = false;
    bool jumpHeld = false;
    bool jumpPressed = false;
    bool dashHeld = false;
    bool dashPressed = false;
    bool shootHeld = false;
    bool shootPressed = false;
};

inline Input withPressEdges(Input held, const Input& previous) {
    held.leftPressed = held.left && !previous.left;
    held.rightPressed = held.right && !previous.right;
    held.upPressed = held.up && !previous.up;
    held.downPressed = held.down && !previous.down;
    held.jumpPressed = held.jumpHeld && !previous.jumpHeld;
    held.dashPressed = held.dashHeld && !previous.dashHeld;
    held.shootPressed = held.shootHeld && !previous.shootHeld;
    return held;
}
