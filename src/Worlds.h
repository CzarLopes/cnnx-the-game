#pragma once

#include <array>
#include <string>

// Mundos na ordem do mapa. Os ids dos blocos seguem w<mundo>_f<fase>_b<bloco>.
struct WorldInfo {
    const char* id;
    const char* name;
    // Mundo com fases em desenvolvimento; os outros aparecem no mapa como "Em breve".
    bool open;
};

constexpr int kWorldCount = 4;
// Seis fases com boss e, por ultimo, a fase final do mundo.
constexpr int kPhasesPerWorld = 7;
constexpr int kFinalPhase = kPhasesPerWorld - 1;

const std::array<WorldInfo, kWorldCount>& worlds();

// "w1_f2" para o mundo 0 e a fase 1.
std::string phaseId(int world, int phase);
// "w1_f2" para o bloco "w1_f2_b3"; vazio se o id nao seguir o padrao.
std::string phaseOfLevel(const std::string& levelId);
