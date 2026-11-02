#pragma once

#include <string>
#include <vector>

constexpr int kTileSize = 16;

struct Box {
    float x, y, w, h;
};

class Room {
public:
    // Mapa em texto: '#' = bloco solido, '.' = vazio, 'P' = ponto de inicio.
    explicit Room(std::vector<std::string> rows);

    static Room testRoom();

    int widthTiles() const { return width_; }
    int heightTiles() const { return height_; }
    int widthPixels() const { return width_ * kTileSize; }
    int heightPixels() const { return height_ * kTileSize; }

    float spawnX() const { return spawnX_; }
    float spawnFeetY() const { return spawnFeetY_; }

    // Fora dos limites da sala conta como solido.
    bool isSolid(int tileX, int tileY) const;
    bool overlapsSolid(const Box& box) const;

private:
    std::vector<std::string> rows_;
    int width_ = 0;
    int height_ = 0;
    float spawnX_ = 0.0f;
    float spawnFeetY_ = 0.0f;
};
