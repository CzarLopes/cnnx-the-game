#pragma once

// Estado dos botoes em um passo da simulacao. Os campos "Pressed" valem true
// somente no passo em que o botao foi apertado.
struct Input {
    bool left = false;
    bool right = false;
    bool jumpHeld = false;
    bool jumpPressed = false;
    bool dashHeld = false;
    bool dashPressed = false;
    bool shootHeld = false;
    bool shootPressed = false;
};
