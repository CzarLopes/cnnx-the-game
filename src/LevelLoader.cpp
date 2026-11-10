#include "LevelLoader.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <utility>

using nlohmann::json;

namespace {

constexpr int kExpectedGridSize = kTileSize;

[[noreturn]] void fail(const std::string& levelId, const std::string& message) {
    throw std::runtime_error("Nivel '" + levelId + "': " + message);
}

const json& findByIdentifier(const json& items, const char* key, const char* identifier,
                             const std::string& levelId) {
    for (const json& item : items) {
        if (item.value(key, std::string()) == identifier) {
            return item;
        }
    }
    fail(levelId, std::string("falta ") + identifier);
}

char tileFromIntGrid(int value, const std::string& levelId) {
    switch (value) {
        case 0: return kTileEmpty;
        case 1: return kTileSolid;
        case 2: return kTileLadder;
        case 3: return kTileSpike;
        case 4: return kTileShock;
        case 5: return kTileCrumble;
        default: fail(levelId, "valor desconhecido na camada Tiles: " + std::to_string(value));
    }
}

std::vector<std::string> readRows(const json& layer, const std::string& levelId) {
    const int width = layer.at("__cWid").get<int>();
    const int height = layer.at("__cHei").get<int>();
    if (layer.at("__gridSize").get<int>() != kExpectedGridSize) {
        fail(levelId, "a camada Tiles precisa ter grade de " + std::to_string(kExpectedGridSize) + " px");
    }
    const json& csv = layer.at("intGridCsv");
    if (csv.size() != static_cast<size_t>(width) * height) {
        fail(levelId, "tamanho da camada Tiles nao bate com a largura e a altura");
    }

    std::vector<std::string> rows(height, std::string(width, kTileEmpty));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            rows[y][x] = tileFromIntGrid(csv[y * width + x].get<int>(), levelId);
        }
    }
    return rows;
}

Box entityBox(const json& entity) {
    return {entity.at("px").at(0).get<float>(), entity.at("px").at(1).get<float>(),
            entity.at("width").get<float>(), entity.at("height").get<float>()};
}

std::string stringField(const json& entity, const std::string& name, const std::string& levelId) {
    for (const json& field : entity.at("fieldInstances")) {
        if (field.at("__identifier") == name && field.at("__value").is_string()) {
            return field.at("__value").get<std::string>();
        }
    }
    fail(levelId, "entidade " + entity.at("__identifier").get<std::string>() + " sem o campo " + name);
}

int strengthOf(const json& level, const std::string& levelId) {
    for (const json& field : level.at("fieldInstances")) {
        if (field.at("__identifier") == "strength" && field.at("__value").is_number_integer()) {
            const int strength = field.at("__value").get<int>();
            if (strength < kMinStrength || strength > kMaxStrength) {
                fail(levelId, "campo strength fora do intervalo de 1 a 3");
            }
            return strength;
        }
    }
    fail(levelId, "falta o campo strength");
}

PuzzleKind puzzleOf(const json& level, const std::string& levelId) {
    for (const json& field : level.at("fieldInstances")) {
        if (field.at("__identifier") == "puzzle" && field.at("__value").is_string()) {
            const std::string name = field.at("__value").get<std::string>();
            if (name == "rj45") {
                return PuzzleKind::Rj45;
            }
            if (name == "tomada") {
                return PuzzleKind::Tomada;
            }
            fail(levelId, "puzzle desconhecido: " + name);
        }
    }
    fail(levelId, "falta o campo puzzle");
}

LevelData parseLevelJson(const json& project, const std::string& levelId) {
    const json& level = findByIdentifier(project.at("levels"), "identifier", levelId.c_str(), levelId);
    const json& layers = level.at("layerInstances");
    const json& tiles = findByIdentifier(layers, "__identifier", "Tiles", levelId);
    const json& entities = findByIdentifier(layers, "__identifier", "Entities", levelId);

    std::vector<std::string> rows = readRows(tiles, levelId);
    std::vector<EnemySpawn> enemies;
    std::vector<ToolPickup> tools;
    std::vector<Box> darkZones;
    std::vector<Box> fallingObjects;
    std::vector<HpUpgrade> hpUpgrades;
    std::optional<Box> door;
    bool hasStart = false;

    for (const json& entity : entities.at("entityInstances")) {
        const std::string name = entity.at("__identifier").get<std::string>();
        const Box box = entityBox(entity);
        const float centerX = box.x + box.w / 2.0f;

        if (name == "PlayerStart") {
            const int tileX = static_cast<int>(box.x) / kTileSize;
            const int tileY = static_cast<int>(box.y) / kTileSize;
            if (tileY >= static_cast<int>(rows.size()) || tileX >= static_cast<int>(rows[0].size())) {
                fail(levelId, "PlayerStart fora do mapa");
            }
            rows[tileY][tileX] = 'P';
            hasStart = true;
        } else if (name == "Crawler") {
            enemies.push_back({EnemyKind::Crawler, centerX, box.y + box.h});
        } else if (name == "Shooter") {
            enemies.push_back({EnemyKind::Shooter, centerX, box.y + box.h});
        } else if (name == "FlyerPassive") {
            enemies.push_back({EnemyKind::FlyerPassive, centerX, box.y + box.h / 2.0f});
        } else if (name == "FlyerChaser") {
            enemies.push_back({EnemyKind::FlyerChaser, centerX, box.y + box.h / 2.0f});
        } else if (name == "FlyerShooter") {
            enemies.push_back({EnemyKind::FlyerShooter, centerX, box.y + box.h / 2.0f});
        } else if (name == "FallingObject") {
            fallingObjects.push_back(box);
        } else if (name == "HpUpgrade") {
            hpUpgrades.push_back({stringField(entity, "id", levelId), box});
        } else if (name == "Tool") {
            tools.push_back({stringField(entity, "kind", levelId), box, false});
        } else if (name == "Door") {
            door = box;
        } else if (name == "DarkZone") {
            darkZones.push_back(box);
        } else {
            fail(levelId, "entidade desconhecida: " + name);
        }
    }

    if (!hasStart) {
        fail(levelId, "falta PlayerStart");
    }
    if (!door) {
        fail(levelId, "falta Door");
    }
    const int strength = strengthOf(level, levelId);
    const PuzzleKind puzzle = puzzleOf(level, levelId);
    return {Room(std::move(rows)), std::move(enemies), std::move(tools), *door, std::move(darkZones),
            strength, std::move(fallingObjects), std::move(hpUpgrades), puzzle, levelId};
}

}  // namespace

size_t findLevel(const std::vector<LevelData>& levels, const std::string& id) {
    for (size_t i = 0; i < levels.size(); ++i) {
        if (levels[i].id == id) {
            return i;
        }
    }
    return 0;
}

LevelData parseLevel(const std::string& ldtkJson, const std::string& levelId) {
    try {
        return parseLevelJson(json::parse(ldtkJson), levelId);
    } catch (const json::exception& error) {
        fail(levelId, std::string("JSON invalido: ") + error.what());
    }
}

LevelData loadLevel(const std::string& ldtkPath, const std::string& levelId) {
    std::ifstream file(ldtkPath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Nao foi possivel abrir " + ldtkPath);
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return parseLevel(contents.str(), levelId);
}
