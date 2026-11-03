#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Enemy.h"
#include "Game.h"
#include "Hazards.h"
#include "LevelLoader.h"
#include "PuzzleRj45.h"
#include "check.h"

namespace {

Input pressed(Input held, const Input& previous) {
    return withPressEdges(held, previous);
}

LevelData arena(std::vector<EnemySpawn> enemies, int strength = 1, std::vector<ToolPickup> tools = {}) {
    std::vector<std::string> rows;
    rows.push_back(std::string(40, '#'));
    for (int i = 0; i < 12; ++i) {
        rows.push_back("#" + std::string(38, '.') + "#");
    }
    rows.push_back("#..P" + std::string(35, '.') + "#");
    rows.push_back(std::string(40, '#'));
    return {Room(rows), std::move(enemies), std::move(tools), {300, 192, 16, 32}, {}, strength};
}

struct Driver {
    Game game;
    Input previous;

    explicit Driver(LevelData level) : game(std::move(level)) {}

    void step(Input held) {
        game.step(pressed(held, previous));
        previous = held;
    }
    void steps(int count, Input held = {}) {
        for (int i = 0; i < count; ++i) {
            step(held);
        }
    }
};

void solvePuzzle(Driver& driver) {
    const auto tap = [&driver](Input held) {
        driver.step(held);
        driver.step({});
    };
    Input left;
    left.left = true;
    Input right;
    right.right = true;
    Input select;
    select.jumpHeld = true;

    for (int slot = 0; slot < PuzzleRj45::kWireCount; ++slot) {
        if (driver.game.puzzle().wireAt(slot) == slot) {
            continue;
        }
        int from = slot;
        while (driver.game.puzzle().wireAt(from) != slot) {
            ++from;
        }
        while (driver.game.puzzle().cursor() != from) {
            tap(driver.game.puzzle().cursor() > from ? left : right);
        }
        tap(select);
        while (driver.game.puzzle().cursor() != slot) {
            tap(left);
        }
        tap(select);
    }
    Input test;
    test.shootHeld = true;
    tap(test);
}

void testForceTable() {
    for (int strength = kMinStrength; strength <= kMaxStrength; ++strength) {
        const ForceStats& stats = forceStats(strength);
        Driver driver(arena({{EnemyKind::Crawler, 400, 224}}, strength));
        Input fire;
        fire.shootHeld = true;

        int hits = 0;
        while (!driver.game.enemies().empty() && hits < 20) {
            const int hp = driver.game.enemies()[0].hp;
            driver.step(fire);
            driver.step({});
            for (int i = 0; i < 60 && !driver.game.enemies().empty() && driver.game.enemies()[0].hp == hp; ++i) {
                driver.step({});
            }
            ++hits;
        }
        CHECK(hits == stats.hp, "forca %d cai com %d tiros normais (%d)", strength, stats.hp, hits);

        Driver contact(arena({{EnemyKind::Crawler, 60, 224}}, strength));
        int hp = contact.game.player().hp();
        for (int i = 0; i < 40 && contact.game.player().hp() == hp; ++i) {
            contact.step({});
        }
        CHECK(hp - contact.game.player().hp() == stats.contactDamage, "forca %d: contato causa %d", strength, stats.contactDamage);

        Driver ranged(arena({{EnemyKind::Shooter, 200, 224}}, strength));
        hp = ranged.game.player().hp();
        for (int i = 0; i < 300 && ranged.game.player().hp() == hp; ++i) {
            ranged.step({});
        }
        CHECK(hp - ranged.game.player().hp() == stats.projectileDamage, "forca %d: projetil causa %d", strength, stats.projectileDamage);
    }
}

void testEnemyBehavior() {
    const Room room({
        "##########",
        "#P.......#",
        "#...##...#",
        "##########",
    });
    std::vector<EnemyShot> noShots;
    std::vector<Enemy> crawler = {makeEnemy({EnemyKind::Crawler, 72, 32}, 1)};
    bool turned = false;
    bool leftPlatform = false;
    for (int i = 0; i < 300; ++i) {
        updateEnemies(crawler, noShots, room, {-500, -500, 14, 28}, 1);
        turned = turned || crawler[0].dir > 0;
        leftPlatform = leftPlatform || crawler[0].box.y != 16.0f;
    }
    CHECK(turned && !leftPlatform, "rastejante vira na beirada sem cair");

    std::vector<Enemy> flyer = {makeEnemy({EnemyKind::Flyer, 40, 24}, 1)};
    const float startX = flyer[0].box.x;
    for (int i = 0; i < 20; ++i) {
        updateEnemies(flyer, noShots, room, {80, 16, 14, 28}, 1);
    }
    CHECK(flyer[0].box.x > startX, "voador persegue o jogador por perto");

    std::vector<Enemy> walled = {makeEnemy({EnemyKind::Flyer, 56, 24}, 1)};
    for (int i = 0; i < 200; ++i) {
        updateEnemies(walled, noShots, room, {100, 34, 14, 14}, 1);
    }
    CHECK(!room.overlapsSolid(walled[0].box), "voador nao atravessa blocos");
}

void testHazards() {
    Room room({
        "##########",
        "#P.......#",
        "#.CC.....#",
        "#..^~....#",
        "##########",
    });
    Hazards hazards(room, {{32, 16, 32, 16}});
    CHECK(hazards.damageAt(room, {48, 20, 14, 28}) == 4, "espinho causa 4");
    CHECK(hazards.damageAt(room, {16, 36, 14, 12}) == 0, "tile ao lado do espinho nao causa dano");

    const Box onShock = {64, 20, 14, 28};
    CHECK(hazards.shockOn() && hazards.damageAt(room, onShock) == 3, "choque ligado causa 3");
    for (int i = 0; i < 80; ++i) {
        hazards.update(room, {-100, -100, 1, 1}, false);
    }
    CHECK(!hazards.shockOn() && hazards.damageAt(room, onShock) == 0, "choque desligado nao causa dano");

    const Box onCrumble = {34, 4, 14, 28};
    hazards.update(room, onCrumble, true);
    for (int i = 0; i < 28; ++i) {
        hazards.update(room, onCrumble, true);
    }
    CHECK(room.tileAt(2, 2) == kTileCrumble, "bloco ainda firme antes de 0,5 s");
    hazards.update(room, onCrumble, true);
    hazards.update(room, onCrumble, true);
    CHECK(room.tileAt(2, 2) == kTileEmpty, "bloco cai em 0,5 s");
    for (int i = 0; i < 185; ++i) {
        hazards.update(room, {-100, -100, 1, 1}, false);
    }
    CHECK(room.tileAt(2, 2) == kTileCrumble, "bloco volta em 3 s");

    CHECK(hazards.darknessAt(40, 20, 40, 20) == 0.0f, "escuro: luz ao redor do jogador");
    CHECK(hazards.darknessAt(40, 20, 160, 20) == 1.0f, "escuro: zona longe do jogador");
    CHECK(hazards.darknessAt(100, 60, 160, 60) == 0.0f, "fora da zona nao escurece");
}

void testRespawn() {
    std::vector<ToolPickup> tools = {{"Crimper", {56, 208, 16, 16}, false}};
    Driver driver(arena({{EnemyKind::Crawler, 200, 224}, {EnemyKind::Crawler, 300, 224}}, 1, tools));
    driver.steps(3);
    CHECK(driver.game.toolsMissing() == 0, "tocar a ferramenta coleta");

    Input fire;
    fire.shootHeld = true;
    driver.step(fire);
    for (int i = 0; i < 60; ++i) {
        driver.step({});
    }
    driver.step(fire);
    driver.steps(60);
    CHECK(driver.game.enemies().size() == 1, "inimigo eliminado antes de renascer");

    driver.game.respawn();
    CHECK(driver.game.enemies().size() == 2 && driver.game.toolsMissing() == 0, "renascer restaura inimigos e mantem ferramentas");
    CHECK(driver.game.player().hp() == Player::kMaxHp, "renascer restaura a vida");

    driver.game.setStrength(3);
    CHECK(driver.game.enemies()[0].hp == 7, "F3 aplica a forca 3");
}

void testDeath() {
    Driver driver(arena({{EnemyKind::Crawler, 120, 224}}, 3));
    int frames = 0;
    while (!driver.game.player().isDead() && frames < 20000) {
        driver.step({});
        ++frames;
    }
    CHECK(driver.game.player().isDead(), "morre depois de varios toques");
    driver.steps(61);
    CHECK(!driver.game.player().isDead() && driver.game.player().hp() == Player::kMaxHp, "renasce sozinho depois de morrer");
}

void testDoor() {
    std::vector<ToolPickup> tools = {{"Cable", {320, 208, 16, 16}, false}};
    LevelData level = arena({}, 1, tools);
    level.door = {48, 192, 16, 32};
    Driver driver(std::move(level));
    Input up;
    up.up = true;
    driver.step(up);
    CHECK(driver.game.mode() == GameMode::Playing && driver.game.showingToolsMessage(), "porta bloqueada sem ferramentas");

    std::vector<ToolPickup> ready = {{"Cable", {60, 208, 16, 16}, false}};
    LevelData open = arena({}, 1, ready);
    open.door = {48, 192, 16, 32};
    Driver opened(std::move(open));
    opened.steps(2);
    opened.step(up);
    CHECK(opened.game.mode() == GameMode::Puzzle, "com as ferramentas, a porta abre o puzzle");
    solvePuzzle(opened);
    CHECK(opened.game.mode() == GameMode::Complete && opened.game.doorOpen(), "montar o RJ45 conclui o bloco");
}

void testPuzzle() {
    PuzzleRj45 puzzle(7);
    CHECK(puzzle.correctCount() < PuzzleRj45::kWireCount, "puzzle comeca embaralhado");
    puzzle.test();
    CHECK(puzzle.status() == PuzzleRj45::Status::Failed, "teste falha com a ordem errada");

    int wrongOrderRuns = 0;
    for (unsigned seed = 1; seed < 200; ++seed) {
        wrongOrderRuns += PuzzleRj45(seed).correctCount() < PuzzleRj45::kWireCount ? 1 : 0;
    }
    CHECK(wrongOrderRuns == 199, "nenhuma semente comeca resolvida");

    PuzzleRj45 solved(3);
    for (int slot = 0; slot < PuzzleRj45::kWireCount; ++slot) {
        int from = slot;
        while (solved.wireAt(from) != slot) {
            ++from;
        }
        if (from == slot) {
            continue;
        }
        while (solved.cursor() != from) {
            solved.moveCursor(1);
        }
        solved.select();
        while (solved.cursor() != slot) {
            solved.moveCursor(1);
        }
        solved.select();
    }
    solved.test();
    CHECK(solved.status() == PuzzleRj45::Status::Solved, "ordem T568B resolve");
}

bool loadFails(const std::string& text, const std::string& expected) {
    try {
        parseLevel(text, "w1_f1_b1");
    } catch (const std::runtime_error& error) {
        return std::string(error.what()).find(expected) != std::string::npos;
    }
    return false;
}

std::string levelJson(const std::string& entities, const std::string& fields = R"([{"__identifier":"strength","__value":1}])") {
    return R"({"levels":[{"identifier":"w1_f1_b1","fieldInstances":)" + fields + R"(,"layerInstances":[
        {"__identifier":"Entities","entityInstances":[)" + entities + R"(]},
        {"__identifier":"Tiles","__cWid":2,"__cHei":2,"__gridSize":16,"intGridCsv":[0,0,1,1]}
    ]}]})";
}

void testLoader() {
    const std::string start = R"({"__identifier":"PlayerStart","px":[0,0],"width":16,"height":16,"fieldInstances":[]})";
    const std::string door = R"({"__identifier":"Door","px":[16,0],"width":16,"height":16,"fieldInstances":[]})";

    const LevelData level = parseLevel(levelJson(start + "," + door), "w1_f1_b1");
    CHECK(level.room.widthTiles() == 2 && level.room.isSolid(0, 1) && level.strength == 1, "carrega um mapa minimo valido");

    CHECK(loadFails(levelJson(door), "PlayerStart"), "erro claro sem PlayerStart");
    CHECK(loadFails(levelJson(start), "Door"), "erro claro sem Door");
    CHECK(loadFails(levelJson(start + "," + door, "[]"), "strength"), "erro claro sem strength");
    CHECK(loadFails(R"({"levels":[{"identifier":"w1_f1_b1","layerInstances":[]}]})", "Tiles"), "erro claro sem camada Tiles");
    CHECK(loadFails("{nao e json", "JSON"), "erro claro com JSON invalido");
    CHECK(loadFails(levelJson(start + "," + R"({"__identifier":"Boss","px":[0,0],"width":16,"height":16,"fieldInstances":[]})" + "," + door), "Boss"), "erro claro com entidade desconhecida");
}

void testBlockContent() {
    const LevelData level = loadLevel(CNNX_LEVEL_PATH, "w1_f1_b1");
    const auto count = [&level](EnemyKind kind) {
        return std::count_if(level.enemies.begin(), level.enemies.end(),
                             [kind](const EnemySpawn& spawn) { return spawn.kind == kind; });
    };
    CHECK(level.strength == 1 && level.room.widthTiles() == 110, "w1_f1_b1 carrega com forca 1 e 110 tiles");
    CHECK(count(EnemyKind::Crawler) == 4 && count(EnemyKind::Flyer) == 3 && count(EnemyKind::Shooter) == 2, "9 inimigos: 4, 3 e 2");
    CHECK(level.tools.size() == 3 && !level.darkZones.empty(), "3 ferramentas e zona escura");
}

// Bot simples: anda para a direita, atira, pula obstaculos, sobe escada e escala parede.
Input botInput(const Game& game, int frame, int& jumpHold) {
    const Room& room = game.room();
    const Player& player = game.player();
    const Box box = player.bounds();
    Input in;
    in.right = true;
    in.shootHeld = frame % 16 < 2;

    const int centerColumn = static_cast<int>((box.x + box.w / 2.0f) / kTileSize);
    const int aheadColumn = static_cast<int>((box.x + box.w + 8.0f) / kTileSize);
    const int feetRow = static_cast<int>((box.y + box.h) / kTileSize);

    if (room.overlapsTile({box.x + box.w / 2.0f, box.y, 1.0f, box.h}, kTileLadder)) {
        in.up = true;
        return in;
    }

    const char under = room.tileAt(aheadColumn, feetRow);
    const char under2 = room.tileAt(aheadColumn + 1, feetRow);
    const bool hazardAhead = under == kTileSpike || under == kTileShock || under2 == kTileSpike || under2 == kTileShock;
    const bool wallAhead = room.isSolid(aheadColumn, feetRow - 1);

    // So vale escalar uma parede que tenha topo
    bool wallHasTop = false;
    for (int row = feetRow - 1; row > 0; --row) {
        wallHasTop = wallHasTop || !room.isSolid(centerColumn + 1, row);
    }

    if (player.grounded() && (hazardAhead || wallAhead) && jumpHold <= 0) {
        jumpHold = 30;
    }
    if (!player.grounded() && wallHasTop && room.isSolid(centerColumn + 1, feetRow - 1)) {
        jumpHold = frame % 24 < 14 ? 1 : 0;
    }

    in.jumpHeld = jumpHold > 0;
    jumpHold = std::max(jumpHold - 1, 0);

    if (boxesOverlap(box, game.door())) {
        in.right = false;
        in.up = frame % 4 < 2;
    }
    return in;
}

void testBlockCanBeFinished() {
    Driver driver(loadLevel(CNNX_LEVEL_PATH, "w1_f1_b1"));
    int jumpHold = 0;
    int frame = 0;
    int deaths = 0;
    bool wasDead = false;
    int bestX = 0;
    for (; frame < 60 * 60 * 5 && driver.game.mode() == GameMode::Playing; ++frame) {
        driver.step(botInput(driver.game, frame, jumpHold));
        const bool dead = driver.game.player().isDead();
        deaths += dead && !wasDead ? 1 : 0;
        wasDead = dead;
        bestX = std::max(bestX, static_cast<int>(driver.game.player().bounds().x));
    }
    CHECK(driver.game.mode() == GameMode::Puzzle, "bot chega ao puzzle (x maximo=%d, ferramentas faltando=%d, mortes=%d)", bestX, driver.game.toolsMissing(), deaths);
    std::printf("     tempo do bot: %.1f s, mortes: %d\n", frame / 60.0, deaths);
    for (const ToolPickup& tool : driver.game.tools()) {
        if (!tool.collected) {
            std::printf("     ferramenta faltando: %s em x=%.0f y=%.0f\n", tool.kind.c_str(), tool.box.x, tool.box.y);
        }
    }
    if (driver.game.mode() == GameMode::Puzzle) {
        solvePuzzle(driver);
        CHECK(driver.game.mode() == GameMode::Complete, "bot conclui o bloco");
    }
}

}  // namespace

void runGameTests() {
    testForceTable();
    testEnemyBehavior();
    testHazards();
    testRespawn();
    testDeath();
    testDoor();
    testPuzzle();
    testLoader();
    testBlockContent();
    testBlockCanBeFinished();
}
