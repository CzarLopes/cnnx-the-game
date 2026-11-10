#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Game.h"
#include "GameRender.h"
#include "Input.h"
#include "LevelLoader.h"
#include "TitleMenu.h"
#include "raylib.h"

namespace {

constexpr int kStartScale = 3;
constexpr double kStepSeconds = 1.0 / 60.0;
constexpr double kMaxFrameSeconds = 0.25;

Input readHeldInput() {
    Input in;
    in.left = IsKeyDown(KEY_LEFT);
    in.right = IsKeyDown(KEY_RIGHT);
    in.up = IsKeyDown(KEY_UP);
    in.down = IsKeyDown(KEY_DOWN);
    in.jumpHeld = IsKeyDown(KEY_Z);
    in.dashHeld = IsKeyDown(KEY_X);
    in.shootHeld = IsKeyDown(KEY_C);

    if (IsGamepadAvailable(0)) {
        const float axisX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        const float axisY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        in.left = in.left || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) || axisX < -0.5f;
        in.right = in.right || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) || axisX > 0.5f;
        in.up = in.up || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP) || axisY < -0.5f;
        in.down = in.down || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) || axisY > 0.5f;
        in.jumpHeld = in.jumpHeld || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        in.dashHeld = in.dashHeld || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ||
                      IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
        in.shootHeld = in.shootHeld || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
    }
    return in;
}

void handleDevKeys(Game& game) {
#ifdef CNNX_DEV_KEYS
    if (IsKeyPressed(KEY_F1)) {
        game.setStrength(1);
    }
    if (IsKeyPressed(KEY_F2)) {
        game.setStrength(2);
    }
    if (IsKeyPressed(KEY_F3)) {
        game.setStrength(3);
    }
#else
    (void)game;
#endif
}

std::string savePath() {
    const char* appData = std::getenv("APPDATA");
    const std::string folder = appData ? std::string(appData) + "/CnnxResgateDosFreelas/"
                                       : std::string(GetApplicationDirectory());
    return folder + "save.json";
}

TitleMenu makeTitleMenu() {
    return TitleMenu(Profile(savePath()).hasProgress());
}

// Executa a escolha do menu; o perfil e lido do disco para pegar o que o jogo ja salvou.
void startGame(TitleAction action, const std::vector<LevelData>& levels, std::optional<Game>& game) {
    Profile profile(savePath());
    size_t startLevel = 0;
    if (action == TitleAction::NewGame) {
        profile.reset();
    } else {
        startLevel = findLevel(levels, profile.savedLevel());
    }
    game.emplace(levels, std::move(profile), static_cast<unsigned>(std::time(nullptr)), startLevel);
}

int run() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(kViewWidth * kStartScale, kViewHeight * kStartScale, "Cnnx em o Resgate dos Freelas");
    SetWindowMinSize(kViewWidth, kViewHeight);
    // Esc volta para a tela inicial; so fecha o jogo a partir dela
    SetExitKey(KEY_NULL);

    const std::string assetDirectory = std::string(GetApplicationDirectory()) + "assets/";
    loadGameTextures(assetDirectory);

    const std::string levelPath = assetDirectory + "levels/cnnx.ldtk";
    std::vector<LevelData> levels;
    for (const char* id : {"w1_f1_b1", "w1_f1_b2"}) {
        levels.push_back(loadLevel(levelPath, id));
    }
    std::optional<Game> game;
    TitleMenu menu = makeTitleMenu();

    RenderTexture2D target = LoadRenderTexture(kViewWidth, kViewHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    Input previousInput;
    double accumulator = 0.0;
    bool quit = false;

    while (!quit && !WindowShouldClose()) {
        accumulator += std::min(static_cast<double>(GetFrameTime()), kMaxFrameSeconds);
        const Input held = readHeldInput();

        if (game) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                game.reset();
                menu = makeTitleMenu();
            } else {
                if (IsKeyPressed(KEY_R)) {
                    game->respawn();
                }
                handleDevKeys(*game);
            }
        } else if (IsKeyPressed(KEY_ESCAPE) && !menu.back()) {
            quit = true;
        }

        // A simulacao nao roda 1:1 com o desenho, entao as bordas de botao sao
        // calculadas por passo; IsKeyPressed perderia ou repetiria apertos.
        while (accumulator >= kStepSeconds) {
            const Input in = withPressEdges(held, previousInput);
            if (game) {
                game->step(in);
            } else {
                const TitleAction action = menu.step(in);
                if (action != TitleAction::None) {
                    startGame(action, levels, game);
                }
            }
            previousInput = held;
            accumulator -= kStepSeconds;
        }

        BeginTextureMode(target);
        if (game) {
            drawGame(*game);
        } else {
            drawTitle(menu);
        }
        EndTextureMode();

        // Escala inteira para manter o pixel nitido em qualquer tamanho de janela
        const int scale = std::max(std::min(GetScreenWidth() / kViewWidth, GetScreenHeight() / kViewHeight), 1);
        const Rectangle source = {0.0f, 0.0f, static_cast<float>(kViewWidth), -static_cast<float>(kViewHeight)};
        const Rectangle dest = {static_cast<float>((GetScreenWidth() - kViewWidth * scale) / 2),
                                static_cast<float>((GetScreenHeight() - kViewHeight * scale) / 2),
                                static_cast<float>(kViewWidth * scale), static_cast<float>(kViewHeight * scale)};

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(target.texture, source, dest, {0.0f, 0.0f}, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(target);
    unloadGameTextures();
    CloseWindow();
    return 0;
}

}  // namespace

int main() {
    try {
        return run();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Erro: %s\n", error.what());
        return 1;
    }
}
