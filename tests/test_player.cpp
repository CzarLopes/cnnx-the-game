#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "Input.h"
#include "Player.h"
#include "Room.h"
#include "check.h"

namespace {

struct Keys {
    bool left = false;
    bool right = false;
    bool jump = false;
    bool dash = false;
    bool shoot = false;
    bool up = false;
    bool down = false;
};

Keys go(bool right, bool jump = false, bool dash = false) {
    Keys keys;
    keys.right = right;
    keys.jump = jump;
    keys.dash = dash;
    return keys;
}

struct Sim {
    Room room;
    Player player;
    std::vector<Shot> shots;
    Input previous;

    Sim(Room mapRoom, float x, float feet) : room(std::move(mapRoom)) {
        player.spawn(x, feet);
        steps(2, {});
    }

    void step(const Keys& keys) {
        Input held;
        held.left = keys.left;
        held.right = keys.right;
        held.up = keys.up;
        held.down = keys.down;
        held.jumpHeld = keys.jump;
        held.dashHeld = keys.dash;
        held.shootHeld = keys.shoot;
        const Input in = withPressEdges(held, previous);
        player.update(in, room, shots);
        updateShots(shots, room);
        previous = held;
    }

    void steps(int count, const Keys& keys) {
        for (int i = 0; i < count; ++i) {
            step(keys);
        }
    }

    float feet() const { return player.bounds().y + Player::kHeight; }
};

Room flatRoom() {
    std::vector<std::string> rows;
    rows.push_back(std::string(40, '#'));
    for (int i = 0; i < 13; ++i) {
        rows.push_back("#" + std::string(38, '.') + "#");
    }
    rows.push_back("#..P" + std::string(35, '.') + "#");
    rows.push_back(std::string(40, '#'));
    return Room(rows);
}

Room ladderRoom() {
    return Room({
        "####################",
        "#..................#",
        "#..................#",
        "#.......H####......#",
        "#.......H..........#",
        "#.......H..........#",
        "#.......H..........#",
        "#.......H..........#",
        "#.P.....H..........#",
        "####################",
    });
}

void testMovement() {
    Sim run(flatRoom(), 56, 240);
    const float start = run.player.bounds().x;
    run.steps(40, go(true));
    CHECK(run.player.bounds().x - start == 60.0f && run.player.state() == PlayerState::Run, "corrida a 1.5 px por passo");

    Sim jump(flatRoom(), 56, 240);
    const float ground = jump.player.bounds().y;
    float top = ground;
    int air = 0;
    do {
        jump.step(go(false, true));
        ++air;
        top = std::min(top, jump.player.bounds().y);
    } while (!jump.player.grounded() && air < 200);
    CHECK(ground - top > 44 && ground - top < 52, "pulo cheio: %.1f px", ground - top);

    Sim hop(flatRoom(), 56, 240);
    top = ground;
    hop.steps(4, go(false, true));
    for (int i = 0; i < 40; ++i) {
        hop.step({});
        top = std::min(top, hop.player.bounds().y);
    }
    CHECK(ground - top < 22, "pulo curto: %.1f px", ground - top);
}

void testDash() {
    Sim dash(flatRoom(), 56, 240);
    const float start = dash.player.bounds().x;
    dash.steps(41, go(false, false, true));
    const float distance = dash.player.bounds().x - start;
    CHECK(dash.player.state() == PlayerState::Idle && distance > 100 && distance < 115, "dash termina sozinho: %.1f px", distance);

    Sim released(flatRoom(), 56, 240);
    released.steps(5, go(false, false, true));
    released.step({});
    CHECK(released.player.state() != PlayerState::Dash, "soltar X encerra o dash");

    Sim opposite(flatRoom(), 300, 240);
    opposite.steps(3, go(true, false, true));
    Keys back = go(false, false, true);
    back.left = true;
    opposite.step(back);
    CHECK(opposite.player.state() != PlayerState::Dash && opposite.player.vx() == -1.5f, "lado oposto cancela o dash");
}

void testAirDash() {
    Sim air(flatRoom(), 56, 240);
    air.steps(10, go(true, true));
    const float height = air.player.bounds().y;
    air.step(go(true, true, true));
    CHECK(air.player.state() == PlayerState::AirDash, "X no ar inicia o dash aereo");

    bool level = true;
    int frames = 1;
    while (air.player.state() == PlayerState::AirDash && frames < 100) {
        air.step(go(true, true, true));
        ++frames;
        level = level && (air.player.state() != PlayerState::AirDash || air.player.bounds().y == height);
    }
    CHECK(level && frames == 20, "dash aereo reto por 20 passos (%d)", frames);

    air.step(go(true));
    air.step(go(true, false, true));
    CHECK(air.player.state() != PlayerState::AirDash, "segundo dash no mesmo pulo e negado");

    for (int i = 0; i < 200 && !air.player.grounded(); ++i) {
        air.step({});
    }
    air.steps(10, go(true, true));
    air.step(go(true, true, true));
    CHECK(air.player.state() == PlayerState::AirDash, "pousar recarrega o dash aereo");

    Sim both(flatRoom(), 56, 240);
    both.step(go(true, true, true));
    CHECK(both.player.state() == PlayerState::Jump && both.player.vx() == 3.5f, "X e Z juntos viram pulo com dash");
}

void testWalls() {
    Sim climb(Room::testRoom(), 37 * 16 - 20, 19 * 16);
    const float start = climb.player.bounds().y;
    bool slid = false;
    bool kicked = false;
    climb.steps(12, go(true, true));
    for (int i = 0; i < 900; ++i) {
        climb.step(go(true, i % 24 < 14));
        slid = slid || climb.player.state() == PlayerState::WallSlide;
        kicked = kicked || climb.player.state() == PlayerState::WallKick;
        if (climb.feet() <= 8 * 16 && climb.player.bounds().x >= 38 * 16) {
            break;
        }
    }
    CHECK(slid && kicked && start - climb.player.bounds().y > 120, "escala uma parede sozinha");
}

void testShots() {
    Keys fire;
    fire.shoot = true;

    Sim normal(flatRoom(), 56, 240);
    normal.step(fire);
    normal.step({});
    CHECK(normal.shots.size() == 1 && shotDamage(normal.shots[0]) == 1, "tiro normal causa 1");
    for (int i = 0; i < 6; ++i) {
        normal.step(fire);
        normal.step({});
    }
    CHECK(normal.shots.size() == 3, "limite de 3 tiros normais");

    Sim charged(flatRoom(), 56, 240);
    charged.steps(35, fire);
    charged.shots.clear();
    charged.step({});
    CHECK(charged.shots.size() == 1 && shotDamage(charged.shots[0]) == 2, "carga media causa 2");
    charged.shots.clear();
    charged.steps(100, fire);
    charged.shots.clear();
    charged.step({});
    CHECK(charged.shots.size() == 1 && shotDamage(charged.shots[0]) == 4, "carga total causa 4");
}

void testLadder() {
    Keys up;
    up.up = true;
    Keys down;
    down.down = true;

    Sim climb(ladderRoom(), 8 * 16 + 8, 9 * 16);
    climb.step(up);
    CHECK(climb.player.state() == PlayerState::Climb, "Cima agarra a escada");
    climb.step({});
    const float stopped = climb.player.bounds().y;
    climb.steps(5, {});
    CHECK(climb.player.bounds().y == stopped, "fica parado na escada sem apertar nada");

    for (int i = 0; i < 200 && climb.player.state() == PlayerState::Climb; ++i) {
        climb.step(up);
    }
    climb.steps(3, {});
    CHECK(climb.player.grounded() && climb.feet() == 3 * 16, "sai no topo e fica em pe (pes=%.1f)", climb.feet());

    climb.step(down);
    CHECK(climb.player.state() == PlayerState::Climb, "Baixo desce pelo topo da escada");
    for (int i = 0; i < 200 && climb.player.state() == PlayerState::Climb; ++i) {
        climb.step(down);
    }
    CHECK(climb.player.grounded() && climb.feet() == 9 * 16, "chega ao chao pela escada");

    Sim release(ladderRoom(), 8 * 16 + 8, 9 * 16);
    release.steps(10, up);
    Keys jump;
    jump.jump = true;
    release.step(jump);
    CHECK(release.player.state() == PlayerState::Fall, "pulo solta da escada");
    const float before = release.feet();
    release.steps(10, {});
    CHECK(release.feet() > before, "cai depois de soltar");
}

void testDamage() {
    Sim sim(flatRoom(), 56, 240);
    CHECK(sim.player.takeDamage(3, -1) && sim.player.hp() == 13, "dano reduz a vida");
    CHECK(sim.player.state() == PlayerState::Hurt, "dano causa recuo");
    CHECK(!sim.player.takeDamage(3, -1) && sim.player.hp() == 13, "invulnerabilidade ignora dano");
    sim.steps(61, {});
    CHECK(!sim.player.invulnerable() && sim.player.state() == PlayerState::Idle, "volta ao normal em cerca de 1 s");
    CHECK(sim.player.takeDamage(13, 1) && sim.player.isDead(), "vida zerada mata");
}

}  // namespace

void runPlayerTests() {
    testMovement();
    testDash();
    testAirDash();
    testWalls();
    testShots();
    testLadder();
    testDamage();
}
