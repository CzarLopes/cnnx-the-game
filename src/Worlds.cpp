#include "Worlds.h"

const std::array<WorldInfo, kWorldCount>& worlds() {
    static const std::array<WorldInfo, kWorldCount> kWorlds = {{
        {"w1", "Planeta Catraca", true},
        {"w2", "Rock in Rede", false},
        {"w3", "Lollaparafuso", false},
        {"w4", "Rock the Morro", false},
    }};
    return kWorlds;
}

std::string phaseId(int world, int phase) {
    return std::string(worlds()[world].id) + "_f" + std::to_string(phase + 1);
}

std::string phaseOfLevel(const std::string& levelId) {
    const size_t block = levelId.rfind("_b");
    return block == std::string::npos ? std::string() : levelId.substr(0, block);
}
