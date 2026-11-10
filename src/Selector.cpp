#include "Selector.h"

#include <algorithm>

Selector::Selector(int count, int cursor) : count_(std::max(count, 1)), cursor_(std::clamp(cursor, 0, count_ - 1)) {}

SelectResult Selector::step(const Input& in) {
    ++frame_;
    if (in.leftPressed || in.upPressed) {
        cursor_ = std::max(cursor_ - 1, 0);
    }
    if (in.rightPressed || in.downPressed) {
        cursor_ = std::min(cursor_ + 1, count_ - 1);
    }
    if (in.jumpPressed) {
        return SelectResult::Confirm;
    }
    if (in.dashPressed) {
        return SelectResult::Back;
    }
    return SelectResult::None;
}
