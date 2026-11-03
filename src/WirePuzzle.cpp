#include "WirePuzzle.h"

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>

WirePuzzle::WirePuzzle(PuzzleKind kind, unsigned seed)
    : kind_(kind), wireCount_(kind == PuzzleKind::Tomada ? 3 : kMaxWires) {
    const auto end = wires_.begin() + wireCount_;
    std::iota(wires_.begin(), end, 0);
    std::mt19937 random(seed);
    std::shuffle(wires_.begin(), end, random);
    if (correctCount() == wireCount_) {
        std::swap(wires_[0], wires_[1]);
    }
}

void WirePuzzle::moveCursor(int dir) {
    if (status_ == Status::Solved) {
        return;
    }
    cursor_ = (cursor_ + dir + wireCount_) % wireCount_;
    status_ = Status::Editing;
}

void WirePuzzle::select() {
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

void WirePuzzle::test() {
    if (status_ == Status::Solved) {
        return;
    }
    held_ = -1;
    status_ = correctCount() == wireCount_ ? Status::Solved : Status::Failed;
}

int WirePuzzle::correctCount() const {
    int correct = 0;
    for (int slot = 0; slot < wireCount_; ++slot) {
        correct += wires_[slot] == slot ? 1 : 0;
    }
    return correct;
}
