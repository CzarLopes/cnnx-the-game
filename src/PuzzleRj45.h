#pragma once

#include <array>

// Cada fio e identificado pela sua posicao correta na ordem T568B:
// branco-laranja, laranja, branco-verde, azul, branco-azul, verde, branco-marrom, marrom.
class PuzzleRj45 {
public:
    static constexpr int kWireCount = 8;
    enum class Status { Editing, Failed, Solved };

    explicit PuzzleRj45(unsigned seed = 1);

    void moveCursor(int dir);
    void select();  // pega o fio do cursor ou troca com o fio que estava na mao
    void test();

    int wireAt(int slot) const { return wires_[slot]; }
    int cursor() const { return cursor_; }
    int held() const { return held_; }  // posicao do fio na mao, ou -1
    Status status() const { return status_; }
    int correctCount() const;  // resultado do testador de cabo

private:
    std::array<int, kWireCount> wires_{};
    int cursor_ = 0;
    int held_ = -1;
    Status status_ = Status::Editing;
};
