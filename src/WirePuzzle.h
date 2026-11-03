#pragma once

#include <array>

// Rj45: ordem T568B, de branco-laranja a marrom.
// Tomada: fase, neutro e terra nos terminais F, N e T.
enum class PuzzleKind { Rj45, Tomada };

// Cada fio e identificado pela sua posicao correta.
class WirePuzzle {
public:
    static constexpr int kMaxWires = 8;
    enum class Status { Editing, Failed, Solved };

    explicit WirePuzzle(PuzzleKind kind = PuzzleKind::Rj45, unsigned seed = 1);

    void moveCursor(int dir);
    void select();  // pega o fio do cursor ou troca com o fio que estava na mao
    void test();

    PuzzleKind kind() const { return kind_; }
    int wireCount() const { return wireCount_; }
    int wireAt(int slot) const { return wires_[slot]; }
    int cursor() const { return cursor_; }
    int held() const { return held_; }  // posicao do fio na mao, ou -1
    Status status() const { return status_; }
    int correctCount() const;  // resultado do testador

private:
    PuzzleKind kind_;
    int wireCount_;
    std::array<int, kMaxWires> wires_{};
    int cursor_ = 0;
    int held_ = -1;
    Status status_ = Status::Editing;
};
