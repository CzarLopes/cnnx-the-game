#include "GameRender.h"

#include <algorithm>
#include <cmath>

#include "raylib.h"
#include "rlgl.h"

namespace {

constexpr Color kSkyColor = {24, 28, 48, 255};
constexpr Color kDotColor = {44, 52, 84, 255};
constexpr Color kTileColor = {62, 92, 130, 255};
constexpr Color kTileEdgeColor = {112, 160, 196, 255};
constexpr Color kLadderColor = {196, 150, 80, 255};
constexpr Color kSpikeColor = {225, 225, 235, 255};
constexpr Color kShockOnColor = {255, 240, 90, 255};
constexpr Color kShockOffColor = {96, 92, 56, 255};
constexpr Color kCrumbleColor = {150, 104, 70, 255};
constexpr Color kPlayerColor = {60, 130, 240, 255};
constexpr Color kDashColor = {120, 220, 255, 255};
constexpr Color kWallColor = {130, 110, 245, 255};
constexpr Color kHurtColor = {255, 90, 90, 255};
constexpr Color kVisorColor = {240, 240, 250, 255};
constexpr Color kChargeMidColor = {250, 220, 90, 255};
constexpr Color kChargeFullColor = {255, 120, 90, 255};
constexpr Color kFallingColor = {205, 170, 110, 255};
constexpr Color kCrawlerColor = {90, 200, 110, 255};
constexpr Color kShooterColor = {240, 150, 60, 255};
constexpr Color kFlyerPassiveColor = {150, 170, 200, 255};
constexpr Color kFlyerChaserColor = {230, 110, 200, 255};
constexpr Color kFlyerShooterColor = {190, 90, 240, 255};
constexpr Color kEnemyShotColor = {255, 70, 70, 255};
constexpr Color kToolColor = {120, 245, 210, 255};
constexpr Color kUpgradeColor = {255, 215, 0, 255};
constexpr Color kHealSmallColor = {255, 170, 180, 255};
constexpr Color kHealMediumColor = {255, 110, 130, 255};
constexpr Color kHealLargeColor = {240, 50, 80, 255};
constexpr Color kDoorClosedColor = {170, 60, 60, 255};
constexpr Color kDoorOpenColor = {70, 180, 90, 255};
constexpr Color kPanelColor = {14, 16, 28, 255};
constexpr Color kBarBackColor = {30, 30, 40, 255};
constexpr Color kBarColor = {90, 230, 120, 255};

constexpr int kDarkCell = 4;
constexpr int kCrumbleFallFrames = 24;

struct Camera2i {
    int x;
    int y;
};

Rectangle snapped(const Box& box, const Camera2i& cam) {
    return {static_cast<float>(static_cast<int>(box.x) - cam.x), static_cast<float>(static_cast<int>(box.y) - cam.y),
            box.w, box.h};
}

void drawBox(const Box& box, const Camera2i& cam, Color color) {
    DrawRectangleRec(snapped(box, cam), color);
}

void drawSpike(int px, int py) {
    const float left = static_cast<float>(px);
    const float base = static_cast<float>(py + kTileSize);
    for (int i = 0; i < 2; ++i) {
        const float x = left + static_cast<float>(i * 8);
        DrawTriangle({x + 4.0f, static_cast<float>(py)}, {x, base}, {x + 8.0f, base}, kSpikeColor);
    }
}

void drawTile(const Game& game, char tile, int x, int y, int px, int py) {
    const Room& room = game.room();
    switch (tile) {
        case kTileSolid:
            DrawRectangle(px, py, kTileSize, kTileSize, kTileColor);
            if (!room.isSolid(x, y - 1)) {
                DrawRectangle(px, py, kTileSize, 2, kTileEdgeColor);
            }
            break;
        case kTileLadder:
            DrawRectangle(px + 3, py, 2, kTileSize, kLadderColor);
            DrawRectangle(px + 11, py, 2, kTileSize, kLadderColor);
            DrawRectangle(px + 3, py + 3, 10, 2, kLadderColor);
            DrawRectangle(px + 3, py + 11, 10, 2, kLadderColor);
            break;
        case kTileSpike:
            drawSpike(px, py);
            break;
        case kTileShock:
            DrawRectangle(px, py, kTileSize, kTileSize, kTileColor);
            DrawRectangle(px, py, kTileSize, 4, game.hazards().shockOn() ? kShockOnColor : kShockOffColor);
            break;
        default:
            if (x % 2 == 0 && y % 2 == 0) {
                DrawRectangle(px, py, 2, 2, kDotColor);
            }
            break;
    }
}

void drawRoom(const Game& game, const Camera2i& cam) {
    const Room& room = game.room();
    const int lastX = std::min((cam.x + kViewWidth) / kTileSize, room.widthTiles() - 1);
    const int lastY = std::min((cam.y + kViewHeight) / kTileSize, room.heightTiles() - 1);

    for (int y = cam.y / kTileSize; y <= lastY; ++y) {
        for (int x = cam.x / kTileSize; x <= lastX; ++x) {
            drawTile(game, room.tileAt(x, y), x, y, x * kTileSize - cam.x, y * kTileSize - cam.y);
        }
    }

    for (const CrumbleBlock& block : game.hazards().crumbles()) {
        if (block.phase == CrumbleBlock::Phase::Fallen && block.fallFrames > kCrumbleFallFrames) {
            continue;
        }
        const int shake = block.phase == CrumbleBlock::Phase::Shaking ? (block.timer / 2 % 2) * 2 - 1 : 0;
        const int drop = block.phase == CrumbleBlock::Phase::Fallen ? block.fallFrames * block.fallFrames / 8 : 0;
        DrawRectangle(block.tileX * kTileSize - cam.x + shake, block.tileY * kTileSize - cam.y + drop, kTileSize,
                      kTileSize, kCrumbleColor);
    }

    for (const FallingObject& object : game.hazards().fallingObjects()) {
        if (object.phase == FallingObject::Phase::Gone) {
            continue;
        }
        Box box = object.box;
        if (object.phase == FallingObject::Phase::Warning) {
            box.x += static_cast<float>((object.timer / 2 % 2) * 2 - 1);
        }
        drawBox(box, cam, kFallingColor);
    }
}

void drawDoor(const Game& game, const Camera2i& cam) {
    drawBox(game.door(), cam, game.doorOpen() ? kDoorOpenColor : kDoorClosedColor);
}

void drawTools(const Game& game, const Camera2i& cam) {
    for (const ToolPickup& tool : game.tools()) {
        if (!tool.collected) {
            drawBox(tool.box, cam, kToolColor);
        }
    }
    for (const HpUpgrade& upgrade : game.hpUpgrades()) {
        if (!game.profile().hasUpgrade(upgrade.id)) {
            drawBox(upgrade.box, cam, kUpgradeColor);
        }
    }
}

void drawHealItems(const Game& game, const Camera2i& cam) {
    for (const HealItem& item : game.healItems()) {
        // Pisca quando esta para sumir
        if (item.life < 120 && game.frame() % 8 < 3) {
            continue;
        }
        const Color color = item.size == HealSize::Small    ? kHealSmallColor
                            : item.size == HealSize::Medium ? kHealMediumColor
                                                            : kHealLargeColor;
        drawBox(item.box, cam, color);
    }
}

Color enemyColor(EnemyKind kind) {
    switch (kind) {
        case EnemyKind::Crawler: return kCrawlerColor;
        case EnemyKind::Shooter: return kShooterColor;
        case EnemyKind::FlyerPassive: return kFlyerPassiveColor;
        case EnemyKind::FlyerChaser: return kFlyerChaserColor;
        case EnemyKind::FlyerShooter: return kFlyerShooterColor;
    }
    return WHITE;
}

void drawEnemies(const Game& game, const Camera2i& cam) {
    for (const Enemy& enemy : game.enemies()) {
        drawBox(enemy.box, cam, enemyColor(enemy.kind));
        const int eyeX = static_cast<int>(enemy.box.x) - cam.x + (enemy.dir > 0 ? static_cast<int>(enemy.box.w) - 4 : 1);
        DrawRectangle(eyeX, static_cast<int>(enemy.box.y) - cam.y + 3, 3, 3, BLACK);
    }
    for (const EnemyShot& shot : game.enemyShots()) {
        drawBox(shot.box, cam, kEnemyShotColor);
    }
}

Color playerColor(const Player& player, int frame) {
    Color color = kPlayerColor;
    switch (player.state()) {
        case PlayerState::Dash:
        case PlayerState::AirDash:
            color = kDashColor;
            break;
        case PlayerState::WallSlide:
        case PlayerState::WallKick:
            color = kWallColor;
            break;
        case PlayerState::Hurt:
        case PlayerState::Dead:
            color = kHurtColor;
            break;
        default:
            break;
    }
    // Pisca enquanto carrega, mudando de cor em cada nivel
    if (player.charge() >= Player::kChargeLevel1 && frame % 6 < 3) {
        color = player.charge() >= Player::kChargeLevel2 ? kChargeFullColor : kChargeMidColor;
    }
    return color;
}

void drawPlayer(const Game& game, const Camera2i& cam) {
    const Player& player = game.player();
    if (player.invulnerable() && game.frame() % 6 < 2) {
        return;
    }
    const Box box = player.bounds();
    drawBox(box, cam, playerColor(player, game.frame()));
    const int px = static_cast<int>(box.x) - cam.x;
    const int visorX = player.facing() > 0 ? px + static_cast<int>(box.w) - 6 : px;
    DrawRectangle(visorX, static_cast<int>(box.y) - cam.y + 6, 6, 4, kVisorColor);
}

void drawShots(const Game& game, const Camera2i& cam) {
    for (const Shot& shot : game.shots()) {
        const Color color = shot.level == 2 ? kChargeFullColor : shot.level == 1 ? kChargeMidColor : kVisorColor;
        drawBox(shotBox(shot), cam, color);
    }
}

// Escuro em blocos de 4 px: combina com o pixel art e nao precisa de shader
void drawDarkness(const Game& game, const Camera2i& cam) {
    const Hazards& hazards = game.hazards();
    if (hazards.darkZones().empty()) {
        return;
    }
    const Box box = game.player().bounds();
    const float playerX = box.x + box.w / 2.0f;
    const float playerY = box.y + box.h / 2.0f;

    // Escurece so a cor: o alfa da textura precisa continuar opaco
    rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ZERO, RL_ONE, RL_FUNC_ADD, RL_FUNC_ADD);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);
    for (int y = 0; y < kViewHeight; y += kDarkCell) {
        for (int x = 0; x < kViewWidth; x += kDarkCell) {
            const float darkness = hazards.darknessAt(static_cast<float>(cam.x + x + kDarkCell / 2),
                                                      static_cast<float>(cam.y + y + kDarkCell / 2), playerX, playerY);
            if (darkness > 0.0f) {
                DrawRectangle(x, y, kDarkCell, kDarkCell, {0, 0, 0, static_cast<unsigned char>(darkness * 250.0f)});
            }
        }
    }
    EndBlendMode();
}

// Vida em uma coluna so, que cresce para cima; o lado fica livre para a barra de especial.
// Acima de 66 blocos eles afinam para caber os 100 na altura da tela.
void drawHpBar(const Player& player) {
    constexpr int kBarX = 6;
    constexpr int kBarBottom = kViewHeight - 20;
    constexpr int kThickLimit = 66;

    const int step = player.maxHp() > kThickLimit ? 2 : 3;
    for (int block = 0; block < player.maxHp(); ++block) {
        const int y = kBarBottom - (block + 1) * step;
        DrawRectangle(kBarX, y, 6, step - 1, block < player.hp() ? kBarColor : kBarBackColor);
    }
}

void drawHud(const Game& game) {
    const Player& player = game.player();
    drawHpBar(player);

    const int total = static_cast<int>(game.tools().size());
    DrawText(TextFormat("Forca %d", game.strength()), 30, 4, 10, RAYWHITE);
    DrawText(TextFormat("Ferramentas %d/%d", total - game.toolsMissing(), total), 150, 4, 10, RAYWHITE);
    DrawText(playerStateName(player.state()), 6, kViewHeight - 14, 10, RAYWHITE);

    if (game.showingToolsMessage()) {
        const char* text = TextFormat("Faltam %d ferramentas", game.toolsMissing());
        const int width = MeasureText(text, 10);
        DrawRectangle((kViewWidth - width) / 2 - 4, 40, width + 8, 16, kPanelColor);
        DrawText(text, (kViewWidth - width) / 2, 43, 10, RAYWHITE);
    }
    if (game.showingUpgradeMessage()) {
        const char* text = TextFormat("Vida maxima +%d!", Profile::kHpPerUpgrade);
        const int width = MeasureText(text, 10);
        DrawRectangle((kViewWidth - width) / 2 - 4, 60, width + 8, 16, kPanelColor);
        DrawText(text, (kViewWidth - width) / 2, 63, 10, kUpgradeColor);
    }
}

Color wireColor(int wire) {
    static constexpr Color kColors[4] = {
        {235, 130, 40, 255},
        {60, 180, 80, 255},
        {50, 100, 230, 255},
        {150, 95, 50, 255},
    };
    // Na ordem T568B, so azul e branco-azul invertem o padrao branco-cor
    static constexpr int kColorIndex[WirePuzzle::kMaxWires] = {0, 0, 1, 2, 2, 1, 3, 3};
    return kColors[kColorIndex[wire]];
}

bool wireIsStriped(int wire) {
    return wire == 0 || wire == 2 || wire == 4 || wire == 6;
}

void drawRj45Wire(int wire, int x, int y, int width, int height) {
    DrawRectangle(x, y, width, height, wireIsStriped(wire) ? RAYWHITE : wireColor(wire));
    if (wireIsStriped(wire)) {
        for (int stripe = 0; stripe < height; stripe += 12) {
            DrawRectangle(x, y + stripe, width, std::min(5, height - stripe), wireColor(wire));
        }
    }
}

void drawTomadaWire(int wire, int x, int y, int width, int height) {
    // Fase, neutro e terra
    static constexpr Color kColors[3] = {{220, 50, 50, 255}, {50, 100, 230, 255}, {60, 180, 80, 255}};
    DrawRectangle(x, y, width, height, kColors[wire]);
}

void drawPuzzle(const Game& game) {
    constexpr int kWireY = 60;
    constexpr int kWireHeight = 96;

    const WirePuzzle& puzzle = game.puzzle();
    const bool tomada = puzzle.kind() == PuzzleKind::Tomada;
    const int stepX = tomada ? 48 : 22;
    const int width = tomada ? 24 : 12;
    const int firstX = (kViewWidth - (puzzle.wireCount() - 1) * stepX - width) / 2;

    ClearBackground(kPanelColor);
    const char* title = tomada ? "Monte a tomada" : "Monte o RJ45";
    DrawText(title, (kViewWidth - MeasureText(title, 20)) / 2, 18, 20, RAYWHITE);

    for (int slot = 0; slot < puzzle.wireCount(); ++slot) {
        const int x = firstX + slot * stepX;
        const int y = kWireY - (puzzle.held() == slot ? 8 : 0);
        if (tomada) {
            drawTomadaWire(puzzle.wireAt(slot), x, y, width, kWireHeight);
            DrawText(TextFormat("%c", "FNT"[slot]), x + width / 2 - 3, kWireY - 22, 10, RAYWHITE);
        } else {
            drawRj45Wire(puzzle.wireAt(slot), x, y, width, kWireHeight);
        }
        if (puzzle.cursor() == slot) {
            DrawRectangle(x, kWireY + kWireHeight + 4, width, 3, kChargeMidColor);
        }
    }

    if (puzzle.status() == WirePuzzle::Status::Failed) {
        DrawText(TextFormat("Falha: %d de %d certos", puzzle.correctCount(), puzzle.wireCount()), 58, 172, 10, kHurtColor);
    }
    DrawText("Setas movem   Z pega/troca   C testa", 20, 196, 10, RAYWHITE);
}

void drawComplete(const Game& game) {
    ClearBackground(kPanelColor);
    DrawText("Bloco concluido!", 34, 80, 20, kDoorOpenColor);
    const char* next = game.hasNextLevel() ? "Porta aberta. Z vai para o proximo bloco" : "Fim dos blocos prontos. Z recomeca";
    DrawText(next, (kViewWidth - MeasureText(next, 10)) / 2, 120, 10, RAYWHITE);
}

}  // namespace

void drawGame(const Game& game) {
    if (game.mode() == GameMode::Puzzle) {
        drawPuzzle(game);
        return;
    }
    if (game.mode() == GameMode::Complete) {
        drawComplete(game);
        return;
    }

    const Room& room = game.room();
    const Box box = game.player().bounds();
    const Camera2i cam = {std::clamp(static_cast<int>(box.x + box.w / 2.0f) - kViewWidth / 2, 0,
                                     room.widthPixels() - kViewWidth),
                          std::clamp(static_cast<int>(box.y + box.h / 2.0f) - kViewHeight / 2, 0,
                                     room.heightPixels() - kViewHeight)};

    ClearBackground(kSkyColor);
    drawRoom(game, cam);
    drawDoor(game, cam);
    drawTools(game, cam);
    drawHealItems(game, cam);
    drawEnemies(game, cam);
    drawShots(game, cam);
    drawPlayer(game, cam);
    drawDarkness(game, cam);
    drawHud(game);
}
