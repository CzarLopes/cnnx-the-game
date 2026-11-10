#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Enemy.h"
#include "Game.h"
#include "Hazards.h"
#include "LevelLoader.h"
#include "WirePuzzle.h"
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

    explicit Driver(LevelData level, unsigned seed = 1) : game(std::move(level), Profile(), seed) {}
    explicit Driver(std::vector<LevelData> levels) : game(std::move(levels), Profile(), 1) {}

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

    for (int slot = 0; slot < driver.game.puzzle().wireCount(); ++slot) {
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

    std::vector<Enemy> flyer = {makeEnemy({EnemyKind::FlyerChaser, 40, 24}, 1)};
    const float startX = flyer[0].box.x;
    for (int i = 0; i < 20; ++i) {
        updateEnemies(flyer, noShots, room, {80, 16, 14, 28}, 1);
    }
    CHECK(flyer[0].box.x > startX, "voador perseguidor vem para cima do jogador");

    std::vector<Enemy> walled = {makeEnemy({EnemyKind::FlyerChaser, 56, 24}, 1)};
    for (int i = 0; i < 200; ++i) {
        updateEnemies(walled, noShots, room, {100, 34, 14, 14}, 1);
    }
    CHECK(!room.overlapsSolid(walled[0].box), "voador nao atravessa blocos");

    const Room open({
        "##############################",
        "#P...........................#",
        "#............................#",
        "#............................#",
        "#............................#",
        "#............................#",
        "##############################",
    });
    std::vector<Enemy> passive = {makeEnemy({EnemyKind::FlyerPassive, 200, 48}, 1)};
    const Box nearby = {230, 40, 14, 28};
    float minX = passive[0].box.x;
    float maxX = passive[0].box.x;
    for (int i = 0; i < 600; ++i) {
        updateEnemies(passive, noShots, open, nearby, 1);
        minX = std::min(minX, passive[0].box.x);
        maxX = std::max(maxX, passive[0].box.x);
    }
    CHECK(maxX - minX > 60 && maxX - minX < 90, "voador passivo patrulha e ignora o jogador (%.1f px)", maxX - minX);
    CHECK(noShots.empty(), "voador passivo nao atira");

    std::vector<Enemy> sniper = {makeEnemy({EnemyKind::FlyerShooter, 200, 32}, 1)};
    std::vector<EnemyShot> aimed;
    const float hoverX = sniper[0].box.x;
    for (int i = 0; i < 40 && aimed.empty(); ++i) {
        updateEnemies(sniper, aimed, open, {120, 66, 14, 14}, 1);
    }
    CHECK(aimed.size() == 1 && aimed[0].vx < 0 && aimed[0].vy > 0, "voador atirador mira na diagonal");
    CHECK(sniper[0].box.x == hoverX, "voador atirador mantem posicao");
    for (int i = 0; i < 80; ++i) {
        updateEnemyShots(aimed, open);
    }
    CHECK(aimed.empty(), "tiro diagonal some ao bater no chao");
}

void testFallingObject() {
    Room room({
        "##############",
        "#P...........#",
        "#............#",
        "#............#",
        "#............#",
        "#............#",
        "##############",
    });
    Hazards hazards(room, {}, {{96, 16, 32, 16}});
    const Box far = {16, 64, 14, 28};
    for (int i = 0; i < 30; ++i) {
        hazards.update(room, far, true);
    }
    CHECK(hazards.fallingObjects()[0].phase == FallingObject::Phase::Hanging, "objeto fica suspenso com o jogador longe");

    const Box below = {100, 64, 14, 28};
    hazards.update(room, below, true);
    CHECK(hazards.fallingObjects()[0].phase == FallingObject::Phase::Warning, "objeto treme quando o jogador se aproxima");
    CHECK(hazards.damageAt(room, {96, 16, 32, 16}) == 0, "objeto tremendo ainda nao causa dano");

    bool hit = false;
    for (int i = 0; i < 80 && hazards.fallingObjects()[0].phase != FallingObject::Phase::Gone; ++i) {
        hazards.update(room, below, true);
        hit = hit || hazards.damageAt(room, below) == 2;
    }
    CHECK(hit, "objeto caindo causa 2 de dano");
    CHECK(hazards.fallingObjects()[0].phase == FallingObject::Phase::Gone, "objeto some ao bater no chao");

    for (int i = 0; i < 245; ++i) {
        hazards.update(room, far, true);
    }
    CHECK(hazards.fallingObjects()[0].phase == FallingObject::Phase::Hanging, "objeto volta a ficar suspenso");

    hazards.update(room, below, true);
    hazards.reset(room);
    CHECK(hazards.fallingObjects()[0].phase == FallingObject::Phase::Hanging, "renascer rearma o objeto");
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
    CHECK(hazards.damageAt(room, {48, 20, 14, 28}) == 3, "espinho causa 3");
    CHECK(hazards.damageAt(room, {16, 36, 14, 12}) == 0, "tile ao lado do espinho nao causa dano");

    const Box onShock = {64, 20, 14, 28};
    CHECK(hazards.shockOn() && hazards.damageAt(room, onShock) == 2, "choque ligado causa 2");
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
    CHECK(driver.game.player().hp() == 30, "renascer restaura a vida");

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
    CHECK(!driver.game.player().isDead() && driver.game.player().hp() == 30, "renasce sozinho depois de morrer");
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
    for (const PuzzleKind kind : {PuzzleKind::Rj45, PuzzleKind::Tomada}) {
        const char* name = kind == PuzzleKind::Rj45 ? "RJ45" : "tomada";
        WirePuzzle puzzle(kind, 7);
        CHECK(puzzle.correctCount() < puzzle.wireCount(), "%s comeca embaralhado", name);
        puzzle.test();
        CHECK(puzzle.status() == WirePuzzle::Status::Failed, "%s: teste falha com a ordem errada", name);

        int wrongOrderRuns = 0;
        for (unsigned seed = 1; seed < 200; ++seed) {
            const WirePuzzle scrambled(kind, seed);
            wrongOrderRuns += scrambled.correctCount() < scrambled.wireCount() ? 1 : 0;
        }
        CHECK(wrongOrderRuns == 199, "%s: nenhuma semente comeca resolvida", name);

        WirePuzzle solved(kind, 3);
        for (int slot = 0; slot < solved.wireCount(); ++slot) {
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
        CHECK(solved.status() == WirePuzzle::Status::Solved, "%s: ordem certa resolve", name);
    }
    CHECK(WirePuzzle(PuzzleKind::Rj45).wireCount() == 8 && WirePuzzle(PuzzleKind::Tomada).wireCount() == 3, "RJ45 tem 8 fios e tomada tem 3");
}

void testDrops() {
    constexpr int kRolls = 200000;
    std::mt19937 random(42);
    const auto rates = [&random](int strength) {
        std::array<double, 4> counts{};
        for (int i = 0; i < kRolls; ++i) {
            const std::optional<HealSize> drop = rollDrop(strength, random);
            counts[drop ? static_cast<int>(*drop) : 3] += 1.0 / kRolls;
        }
        return counts;
    };
    const auto near = [](double value, double expected) { return std::fabs(value - expected) < 0.01; };

    const std::array<double, 4> low = rates(2);
    CHECK(low[2] == 0.0, "forca 2 nao solta item grande");
    CHECK(near(low[1], 0.4) && near(low[0], 0.42) && near(low[3], 0.18), "forca 2: medio %.2f, pequeno %.2f, nada %.2f", low[1], low[0], low[3]);

    const std::array<double, 4> high = rates(3);
    CHECK(near(high[2], 0.2) && near(high[1], 0.32) && near(high[0], 0.336), "forca 3: grande %.2f, medio %.2f, pequeno %.2f", high[2], high[1], high[0]);

    CHECK(healAmount(HealSize::Small) == 2 && healAmount(HealSize::Medium) == 5 && healAmount(HealSize::Large) == 10, "cura de 2, 5 e 10 blocos");

    Room room({"######", "#P...#", "#....#", "######"});
    std::vector<HealItem> items = {makeHealItem(HealSize::Small, 40, 20)};
    for (int i = 0; i < 40; ++i) {
        updateHealItems(items, room);
    }
    CHECK(items[0].box.y + items[0].box.h == 48.0f, "item cai e para no chao");
    for (int i = 0; i < 500; ++i) {
        updateHealItems(items, room);
    }
    CHECK(items.empty(), "item some depois de um tempo");

    // Toma dano, elimina o inimigo e anda ate o item, em uma semente que solte cura
    Input fire;
    fire.shootHeld = true;
    Input walk;
    walk.right = true;
    bool healed = false;
    for (unsigned seed = 1; seed <= 20 && !healed; ++seed) {
        Driver driver(arena({{EnemyKind::Crawler, 110, 224}}, 1), seed);
        for (int i = 0; i < 200 && driver.game.player().hp() == 30; ++i) {
            driver.step({});
        }
        const int hurtHp = driver.game.player().hp();
        for (int i = 0; i < 400 && !driver.game.enemies().empty(); ++i) {
            driver.step(i % 20 == 0 ? fire : Input{});
        }
        for (int i = 0; i < 300 && !driver.game.healItems().empty(); ++i) {
            driver.step(walk);
        }
        healed = hurtHp < 30 && driver.game.enemies().empty() && driver.game.healItems().empty() && driver.game.player().hp() > hurtHp;
    }
    CHECK(healed, "inimigo eliminado solta cura, e pegar o item recupera a vida");
}

void testProfile() {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "cnnx_teste_perfil" / "save.json";
    std::filesystem::remove_all(path.parent_path());

    Profile profile(path.string());
    CHECK(profile.maxHp() == 30, "perfil novo tem 30 blocos");
    CHECK(profile.addUpgrade("a") && profile.maxHp() == 35, "conquista leva de 30 para 35");
    CHECK(!profile.addUpgrade("a") && profile.maxHp() == 35, "a mesma conquista nao conta duas vezes");

    const Profile reloaded(path.string());
    CHECK(reloaded.hasUpgrade("a") && reloaded.maxHp() == 35, "conquista fica salva no arquivo");

    Profile capped;
    for (int i = 0; i < 40; ++i) {
        capped.addUpgrade(std::to_string(i));
    }
    CHECK(capped.maxHp() == 100, "vida maxima para em 100");

    std::ofstream(path) << "{quebrado";
    CHECK(Profile(path.string()).maxHp() == 30, "save invalido comeca do zero sem quebrar");
    std::filesystem::remove_all(path.parent_path());

    LevelData level = arena({});
    level.hpUpgrades = {{"segredo", {56, 208, 16, 16}}};
    Driver driver(std::move(level));
    driver.steps(3);
    CHECK(driver.game.player().maxHp() == 35 && driver.game.player().hp() == 35, "pegar a conquista aumenta a vida na hora");
    CHECK(driver.game.showingUpgradeMessage(), "conquista mostra aviso");
    driver.game.respawn();
    CHECK(driver.game.player().maxHp() == 35, "conquista continua depois de morrer");
}

void testLevelFlow() {
    std::vector<LevelData> levels;
    levels.push_back(arena({}));
    LevelData second = arena({{EnemyKind::Crawler, 200, 224}}, 2);
    second.puzzle = PuzzleKind::Tomada;
    levels.push_back(std::move(second));
    for (LevelData& level : levels) {
        level.door = {48, 192, 16, 32};
    }

    Driver driver(std::move(levels));
    Input up;
    up.up = true;
    Input jump;
    jump.jumpHeld = true;

    driver.steps(2);
    driver.step(up);
    CHECK(driver.game.hasNextLevel() && driver.game.puzzle().kind() == PuzzleKind::Rj45, "primeiro bloco usa RJ45");
    solvePuzzle(driver);
    CHECK(driver.game.mode() == GameMode::Complete, "primeiro bloco concluido");
    driver.step({});
    driver.step(jump);
    CHECK(driver.game.levelIndex() == 1 && driver.game.mode() == GameMode::Playing, "Z leva ao segundo bloco");
    CHECK(driver.game.strength() == 2 && !driver.game.doorOpen() && driver.game.enemies()[0].hp == 4, "segundo bloco usa forca 2");

    driver.step({});
    driver.step(up);
    CHECK(driver.game.mode() == GameMode::Puzzle && driver.game.puzzle().kind() == PuzzleKind::Tomada, "segundo bloco usa a tomada");
}

void testSecretCanBeReached() {
    const LevelData level = loadLevel(CNNX_LEVEL_PATH, "w1_f1_b2");
    CHECK(level.hpUpgrades.size() == 1, "bloco 2 esconde uma conquista");
    const Box secret = level.hpUpgrades[0].box;

    Player player;
    std::vector<Shot> shots;
    Input previous;
    player.spawn(67 * kTileSize + 9, 16 * kTileSize);
    bool reached = false;
    for (int frame = 0; frame < 900 && !reached; ++frame) {
        // Pula reto ate entrar na passagem e so entao escala a parede dela
        Input held;
        held.left = player.bounds().y < 12 * kTileSize;
        held.jumpHeld = frame < 22 || frame % 24 < 14;
        player.update(withPressEdges(held, previous), level.room, shots);
        previous = held;
        reached = boxesOverlap(player.bounds(), secret);
    }
    CHECK(reached, "conquista alcancada com pulo de parede pela passagem secreta");
}

bool loadFails(const std::string& text, const std::string& expected) {
    try {
        parseLevel(text, "w1_f1_b1");
    } catch (const std::runtime_error& error) {
        return std::string(error.what()).find(expected) != std::string::npos;
    }
    return false;
}

std::string levelJson(const std::string& entities, const std::string& fields = R"([{"__identifier":"strength","__value":1},{"__identifier":"puzzle","__value":"rj45"}])") {
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
    CHECK(level.id == "w1_f1_b1", "o nivel guarda o proprio id");

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
    CHECK(count(EnemyKind::Crawler) == 6 && count(EnemyKind::Shooter) == 4, "6 rastejantes e 4 lancadores");
    CHECK(count(EnemyKind::FlyerPassive) == 3 && count(EnemyKind::FlyerChaser) == 3 && count(EnemyKind::FlyerShooter) == 2, "voadores: 3 passivos, 3 perseguidores e 2 atiradores");
    CHECK(level.tools.size() == 3 && !level.darkZones.empty() && level.fallingObjects.size() == 5, "3 ferramentas, zona escura e 5 objetos caindo");

    const LevelData second = loadLevel(CNNX_LEVEL_PATH, "w1_f1_b2");
    CHECK(second.strength == 2 && second.puzzle == PuzzleKind::Tomada, "w1_f1_b2 tem forca 2 e tomada");
    CHECK(second.enemies.size() == 21 && second.tools.size() == 3, "w1_f1_b2 tem 21 inimigos e 3 ferramentas");
    const auto secondHas = [&second](EnemyKind kind) {
        return std::any_of(second.enemies.begin(), second.enemies.end(), [kind](const EnemySpawn& spawn) { return spawn.kind == kind; });
    };
    CHECK(secondHas(EnemyKind::FlyerPassive) && secondHas(EnemyKind::FlyerChaser) && secondHas(EnemyKind::FlyerShooter), "w1_f1_b2 tem os tres voadores");
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

    // Para e atira no inimigo a frente; alterna com avancar para nunca travar
    if (player.grounded() && !hazardAhead && frame / 240 % 2 == 0) {
        const float playerX = box.x + box.w / 2.0f;
        const float playerY = box.y + box.h / 2.0f;
        for (const Enemy& enemy : game.enemies()) {
            const float dx = enemy.box.x + enemy.box.w / 2.0f - playerX;
            const float enemyY = enemy.box.y + enemy.box.h / 2.0f;
            const bool clearPath = !room.overlapsSolid({playerX, enemyY, std::max(dx, 1.0f), 1.0f});
            if (dx > 0.0f && dx < 110.0f && std::fabs(enemyY - playerY) < 64.0f && clearPath) {
                in.right = false;
                in.shootHeld = frame % 8 < 2;
                if (enemyY < box.y && jumpHold <= 0) {
                    jumpHold = 20;
                }
                break;
            }
        }
    }

    // Espera o objeto que esta tremendo ou caindo a frente, como uma pessoa faria
    for (const FallingObject& object : game.hazards().fallingObjects()) {
        const bool active = object.phase == FallingObject::Phase::Warning || object.phase == FallingObject::Phase::Falling;
        const float gap = object.box.x - (box.x + box.w);
        if (active && player.grounded() && gap > 0.0f && gap < 32.0f) {
            in.right = false;
        }
    }

    in.jumpHeld = jumpHold > 0;
    jumpHold = std::max(jumpHold - 1, 0);

    if (boxesOverlap(box, game.door())) {
        in.right = false;
        in.up = frame % 4 < 2;
    }
    return in;
}

struct BotRun {
    int frames = 0;
    int deaths = 0;
    int bestX = 0;
};

BotRun playBot(Driver& driver) {
    BotRun run;
    int jumpHold = 0;
    bool wasDead = false;
    for (; run.frames < 60 * 60 * 5 && driver.game.mode() == GameMode::Playing; ++run.frames) {
        driver.step(botInput(driver.game, run.frames, jumpHold));
        const bool dead = driver.game.player().isDead();
        run.deaths += dead && !wasDead ? 1 : 0;
        wasDead = dead;
        run.bestX = std::max(run.bestX, static_cast<int>(driver.game.player().bounds().x));
    }
    return run;
}

// Prova que o caminho existe. A rodada com inimigos so informa o desgaste: o bot
// joga mal de proposito, e a dificuldade real e calibrada jogando.
void testBlockCanBeFinished(const char* levelId) {
    LevelData route = loadLevel(CNNX_LEVEL_PATH, levelId);
    route.enemies.clear();
    Driver driver(std::move(route));
    const BotRun run = playBot(driver);
    CHECK(driver.game.mode() == GameMode::Puzzle, "%s: bot percorre o bloco com todos os perigos (x maximo=%d, ferramentas faltando=%d)", levelId, run.bestX, driver.game.toolsMissing());
    if (driver.game.mode() == GameMode::Puzzle) {
        solvePuzzle(driver);
        CHECK(driver.game.mode() == GameMode::Complete, "%s: bot conclui o bloco", levelId);
    }

    Driver full(loadLevel(CNNX_LEVEL_PATH, levelId));
    const BotRun fullRun = playBot(full);
    std::printf("     %s, bot com inimigos: x maximo=%d de %d, %d mortes em %.0f s, chegou=%d\n", levelId, fullRun.bestX, full.game.room().widthPixels(), fullRun.deaths, fullRun.frames / 60.0, full.game.mode() == GameMode::Puzzle);
}

}  // namespace

void runGameTests() {
    testForceTable();
    testEnemyBehavior();
    testFallingObject();
    testHazards();
    testRespawn();
    testDeath();
    testDoor();
    testPuzzle();
    testDrops();
    testProfile();
    testLevelFlow();
    testSecretCanBeReached();
    testLoader();
    testBlockContent();
    testBlockCanBeFinished("w1_f1_b1");
    testBlockCanBeFinished("w1_f1_b2");
}
