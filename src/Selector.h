#pragma once

#include "Input.h"

enum class SelectResult { None, Confirm, Back };

// Cursor numa lista de itens, usado no mapa de mundos e na escolha de fases.
// Esquerda/cima voltam, direita/baixo avancam, Z confirma e X volta.
// O cursor passa por itens bloqueados; quem usa decide se o Confirm vale.
class Selector {
public:
    explicit Selector(int count = 1, int cursor = 0);

    SelectResult step(const Input& in);

    int cursor() const { return cursor_; }
    int count() const { return count_; }
    int frame() const { return frame_; }

private:
    int count_;
    int cursor_;
    int frame_ = 0;
};
