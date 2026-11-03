#pragma once

#include <cstdio>

extern int gFailures;

#define CHECK(condition, ...)                  \
    do {                                       \
        const bool passed = (condition);       \
        std::printf(passed ? "ok   " : "FAIL "); \
        std::printf(__VA_ARGS__);              \
        std::printf("\n");                     \
        gFailures += passed ? 0 : 1;           \
    } while (0)

void runPlayerTests();
void runGameTests();
