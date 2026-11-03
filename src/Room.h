#pragma once

#include <string>
#include <vector>

constexpr int kTileSize = 16;

constexpr char kTileEmpty = '.';
constexpr char kTileSolid = '#';
constexpr char kTileLadder = 'H';
constexpr char kTileSpike = '^';
constexpr char kTileShock = '~';
constexpr char kTileCrumble = 'C';

struct Box {
    float x, y, w, h;
};

inline bool boxesOverlap(const Box& a, const Box& b) {
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

class Room {
public:
    // Mapa em texto com os tiles acima, mais 'P' como ponto de inicio.
    explicit Room(std::vector<std::string> rows);

    static Room testRoom();

    int widthTiles() const { return width_; }
    int heightTiles() const { return height_; }
    int widthPixels() const { return width_ * kTileSize; }
    int heightPixels() const { return height_ * kTileSize; }

    float spawnX() const { return spawnX_; }
    float spawnFeetY() const { return spawnFeetY_; }

    // Fora dos limites da sala conta como solido.
    char tileAt(int tileX, int tileY) const;
    void setTile(int tileX, int tileY, char tile);
    bool isSolid(int tileX, int tileY) const;
    bool overlapsSolid(const Box& box) const;
    bool overlapsTile(const Box& box, char tile) const;
    // O topo de uma escada funciona como plataforma de mao unica.
    bool isLadderTop(int tileX, int tileY) const;

private:
    std::vector<std::string> rows_;
    int width_ = 0;
    int height_ = 0;
    float spawnX_ = 0.0f;
    float spawnFeetY_ = 0.0f;
};
