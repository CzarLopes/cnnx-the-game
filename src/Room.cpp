#include "Room.h"

#include <cmath>
#include <stdexcept>
#include <utility>

Room::Room(std::vector<std::string> rows) : rows_(std::move(rows)) {
    if (rows_.empty()) {
        throw std::runtime_error("Sala sem linhas");
    }
    height_ = static_cast<int>(rows_.size());
    width_ = static_cast<int>(rows_[0].size());

    bool foundSpawn = false;
    for (int y = 0; y < height_; ++y) {
        if (static_cast<int>(rows_[y].size()) != width_) {
            throw std::runtime_error("Linha " + std::to_string(y) + " da sala tem largura diferente");
        }
        for (int x = 0; x < width_; ++x) {
            if (rows_[y][x] == 'P') {
                spawnX_ = (x + 0.5f) * kTileSize;
                spawnFeetY_ = (y + 1.0f) * kTileSize;
                foundSpawn = true;
            }
        }
    }
    if (!foundSpawn) {
        throw std::runtime_error("Sala sem ponto de inicio 'P'");
    }
}

Room Room::testRoom() {
    return Room({
        "##################################################",
        "#................................................#",
        "#................................................#",
        "#................................................#",
        "#...........................................######",
        "#...........................................#....#",
        "#...........................................#....#",
        "#...........................................#....#",
        "#.....................................#.....#....#",
        "#.........................####........#.....#....#",
        "#.....................................#.....#....#",
        "#.....................................#.....#....#",
        "#...................####..............#.....#....#",
        "#.....................................#.....#....#",
        "#.....................................#.....#....#",
        "#.............####....................#.....#....#",
        "#.....................................#..........#",
        "#.......###...........................#..........#",
        "#..P....###...........................#..........#",
        "##############################......##############",
        "##############################......##############",
        "##################################################",
    });
}

bool Room::isSolid(int tileX, int tileY) const {
    if (tileX < 0 || tileY < 0 || tileX >= width_ || tileY >= height_) {
        return true;
    }
    return rows_[tileY][tileX] == '#';
}

bool Room::overlapsSolid(const Box& box) const {
    const int left = static_cast<int>(std::floor(box.x / kTileSize));
    const int top = static_cast<int>(std::floor(box.y / kTileSize));
    // ceil - 1: encostar exatamente na borda de um tile nao conta como sobrepor
    const int right = static_cast<int>(std::ceil((box.x + box.w) / kTileSize)) - 1;
    const int bottom = static_cast<int>(std::ceil((box.y + box.h) / kTileSize)) - 1;

    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) {
            if (isSolid(x, y)) {
                return true;
            }
        }
    }
    return false;
}
