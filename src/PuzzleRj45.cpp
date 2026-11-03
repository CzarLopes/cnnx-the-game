#include "PuzzleRj45.h"

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>

PuzzleRj45::PuzzleRj45(unsigned seed) {
    std::iota(wires_.begin(), wires_.end(), 0);
    std::mt19937 random(seed);
    std::shuffle(wires_.begin(), wires_.end(), random);
    if (correctCount() == kWireCount) {
        std::swap(wires_[0], wires_[1]);
    }
}

void PuzzleRj45::moveCursor(int dir) {
    if (status_ == Status::Solved) {
        return;
    }
    cursor_ = (cursor_ + dir + kWireCount) % kWireCount;
    status_ = Status::Editing;
}

void PuzzleRj45::select() {
    if (status_ == Status::Solved) {
        return;
    }
    status_ = Status::Editing;
    if (held_ < 0) {
        held_ = cursor_;
    } else {
        std::swap(wires_[held_], wires_[cursor_]);
        held_ = -1;
    }
}

void PuzzleRj45::test() {
    if (status_ == Status::Solved) {
        return;
    }
    held_ = -1;
    status_ = correctCount() == kWireCount ? Status::Solved : Status::Failed;
}

int PuzzleRj45::correctCount() const {
    int correct = 0;
    for (int slot = 0; slot < kWireCount; ++slot) {
        correct += wires_[slot] == slot ? 1 : 0;
    }
    return correct;
}
