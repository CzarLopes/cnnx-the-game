#include <algorithm>
#include <vector>

#include "Input.h"
#include "Player.h"
#include "Room.h"
#include "raylib.h"

namespace {

constexpr int kViewWidth = 256;
constexpr int kViewHeight = 224;
constexpr int kStartScale = 3;
constexpr double kStepSeconds = 1.0 / 60.0;
constexpr double kMaxFrameSeconds = 0.25;

constexpr Color kSkyColor = {24, 28, 48, 255};
constexpr Color kDotColor = {44, 52, 84, 255};
constexpr Color kTileColor = {62, 92, 130, 255};
constexpr Color kTileEdgeColor = {112, 160, 196, 255};
constexpr Color kPlayerColor = {60, 130, 240, 255};
constexpr Color kDashColor = {120, 220, 255, 255};
constexpr Color kWallColor = {130, 110, 245, 255};
constexpr Color kVisorColor = {240, 240, 250, 255};
constexpr Color kChargeMidColor = {250, 220, 90, 255};
constexpr Color kChargeFullColor = {255, 120, 90, 255};

Input readHeldInput() {
    Input in;
    in.left = IsKeyDown(KEY_LEFT);
    in.right = IsKeyDown(KEY_RIGHT);
    in.jumpHeld = IsKeyDown(KEY_Z);
    in.dashHeld = IsKeyDown(KEY_X);
    in.shootHeld = IsKeyDown(KEY_C);

    if (IsGamepadAvailable(0)) {
        const float axis = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        in.left = in.left || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) || axis < -0.5f;
        in.right = in.right || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) || axis > 0.5f;
        in.jumpHeld = in.jumpHeld || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        in.dashHeld = in.dashHeld || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ||
                      IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
        in.shootHeld = in.shootHeld || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
    }
    return in;
}

// A simulacao nao roda 1:1 com o desenho, entao as bordas de botao sao
// calculadas por passo; IsKeyPressed perderia ou repetiria apertos.
Input withPressEdges(Input held, const Input& previous) {
    held.jumpPressed = held.jumpHeld && !previous.jumpHeld;
    held.dashPressed = held.dashHeld && !previous.dashHeld;
    held.shootPressed = held.shootHeld && !previous.shootHeld;
    return held;
}

void drawRoom(const Room& room, int camX, int camY) {
    const int firstX = camX / kTileSize;
    const int firstY = camY / kTileSize;
    const int lastX = std::min((camX + kViewWidth) / kTileSize, room.widthTiles() - 1);
    const int lastY = std::min((camY + kViewHeight) / kTileSize, room.heightTiles() - 1);

    for (int y = firstY; y <= lastY; ++y) {
        for (int x = firstX; x <= lastX; ++x) {
            const int px = x * kTileSize - camX;
            const int py = y * kTileSize - camY;
            if (room.isSolid(x, y)) {
                DrawRectangle(px, py, kTileSize, kTileSize, kTileColor);
                if (!room.isSolid(x, y - 1)) {
                    DrawRectangle(px, py, kTileSize, 2, kTileEdgeColor);
                }
            } else if (x % 2 == 0 && y % 2 == 0) {
                DrawRectangle(px, py, 2, 2, kDotColor);
            }
        }
    }
}

void drawPlayer(const Player& player, int camX, int camY, int frame) {
    const Box box = player.bounds();
    const int px = static_cast<int>(box.x) - camX;
    const int py = static_cast<int>(box.y) - camY;

    Color color = kPlayerColor;
    if (player.state() == PlayerState::Dash || player.state() == PlayerState::AirDash) {
        color = kDashColor;
    } else if (player.state() == PlayerState::WallSlide || player.state() == PlayerState::WallKick) {
        color = kWallColor;
    }
    // Pisca enquanto carrega, mudando de cor em cada nivel
    if (player.charge() >= Player::kChargeLevel1 && frame % 6 < 3) {
        color = player.charge() >= Player::kChargeLevel2 ? kChargeFullColor : kChargeMidColor;
    }

    DrawRectangle(px, py, static_cast<int>(box.w), static_cast<int>(box.h), color);
    const int visorX = player.facing() > 0 ? px + static_cast<int>(box.w) - 6 : px;
    DrawRectangle(visorX, py + 6, 6, 4, kVisorColor);
}

void drawShots(const std::vector<Shot>& shots, int camX, int camY) {
    for (const Shot& shot : shots) {
        const Box box = shotBox(shot);
        const Color color = shot.level == 2 ? kChargeFullColor : shot.level == 1 ? kChargeMidColor : kVisorColor;
        DrawRectangle(static_cast<int>(box.x) - camX, static_cast<int>(box.y) - camY,
                      static_cast<int>(box.w), static_cast<int>(box.h), color);
    }
}

}  // namespace

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(kViewWidth * kStartScale, kViewHeight * kStartScale, "CNNX - Sala de Teste");
    SetWindowMinSize(kViewWidth, kViewHeight);

    RenderTexture2D target = LoadRenderTexture(kViewWidth, kViewHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    const Room room = Room::testRoom();
    Player player;
    player.spawn(room.spawnX(), room.spawnFeetY());
    std::vector<Shot> shots;

    Input previousInput;
    double accumulator = 0.0;
    int frame = 0;

    while (!WindowShouldClose()) {
        accumulator += std::min(static_cast<double>(GetFrameTime()), kMaxFrameSeconds);
        const Input held = readHeldInput();

        if (IsKeyPressed(KEY_R)) {
            player.spawn(room.spawnX(), room.spawnFeetY());
            shots.clear();
        }

        while (accumulator >= kStepSeconds) {
            player.update(withPressEdges(held, previousInput), room, shots);
            updateShots(shots, room);
            previousInput = held;
            accumulator -= kStepSeconds;
            ++frame;
        }

        const Box box = player.bounds();
        const int camX = std::clamp(static_cast<int>(box.x + box.w / 2.0f) - kViewWidth / 2, 0,
                                    room.widthPixels() - kViewWidth);
        const int camY = std::clamp(static_cast<int>(box.y + box.h / 2.0f) - kViewHeight / 2, 0,
                                    room.heightPixels() - kViewHeight);

        BeginTextureMode(target);
        ClearBackground(kSkyColor);
        drawRoom(room, camX, camY);
        drawShots(shots, camX, camY);
        drawPlayer(player, camX, camY, frame);
        DrawText(playerStateName(player.state()), 6, 4, 10, RAYWHITE);
        DrawText("Z pula  X dash  C tiro  R reinicia", 6, kViewHeight - 14, 10, RAYWHITE);
        EndTextureMode();

        // Escala inteira para manter o pixel nitido em qualquer tamanho de janela
        const int scale = std::max(std::min(GetScreenWidth() / kViewWidth, GetScreenHeight() / kViewHeight), 1);
        const float drawWidth = static_cast<float>(kViewWidth * scale);
        const float drawHeight = static_cast<float>(kViewHeight * scale);
        const Rectangle source = {0.0f, 0.0f, static_cast<float>(kViewWidth), -static_cast<float>(kViewHeight)};
        const Rectangle dest = {static_cast<float>((GetScreenWidth() - kViewWidth * scale) / 2),
                                static_cast<float>((GetScreenHeight() - kViewHeight * scale) / 2),
                                drawWidth, drawHeight};

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(target.texture, source, dest, {0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}
