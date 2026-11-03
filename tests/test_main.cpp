#include "check.h"

int gFailures = 0;

int main() {
    runPlayerTests();
    runGameTests();
    std::printf("%d falha(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
