# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# cnnx-the-game

"Cnnx em o Resgate dos Freelas": plataforma 2D estilo Mega Man X em C++17 com raylib 5.5 e nlohmann_json (baixados pelo CMake via FetchContent). Design, numeros de balanceamento e estrutura de mundos/fases/blocos estao em `docs/design.md`; a lista de arte pendente esta em `docs/sprites.md`.

## Git

- Nao rode `git commit` nem `git push`: o usuario faz os dois ao final de cada task.
- Ao terminar cada task, deixe as mudancas sem commit e passe um resumo basico para a mensagem de commit.

## Comandos (Windows, MinGW-w64)

Instalacao dos requisitos no Windows (winget) e no Linux (apt, dnf, pacman): `docs/setup.md`.

```powershell
cmake -S . -B build -G "MinGW Makefiles"   # primeira vez baixa raylib e json
cmake --build build -j 8
.\build\cnnx.exe
ctest --test-dir build --output-on-failure   # ou rode .\build\cnnx_tests.exe direto
```

- Os testes sao um executavel unico (`cnnx_tests`) com um macro `CHECK` proprio (`tests/check.h`), sem framework. Nao ha filtro por teste: para rodar so um, comente as chamadas em `runPlayerTests()` / `runGameTests()`. A saida lista `ok`/`FAIL` por checagem e o total de falhas.
- `-DCNNX_DEV_KEYS=OFF` desativa F1/F2/F3 (troca de forca do bloco).
- O build copia `assets/` para perto do `.exe`; depois de editar o mapa ou sprites, rode `cmake --build build` de novo.

## Arquitetura

- **`cnnx_core` (biblioteca estatica) nao depende de raylib.** Todas as regras ficam ali para rodar nos testes sem janela. So `src/main.cpp` e `src/GameRender.cpp` usam raylib. Nao inclua `raylib.h` em arquivos do core.
- **Simulacao deterministica em passo fixo de 60 Hz.** `main.cpp` acumula tempo e chama `Game::step(Input)` N vezes por quadro. As bordas de botao (`*Pressed`) sao calculadas por passo com `withPressEdges(held, previous)` (`src/Input.h`), nunca com `IsKeyPressed`. O sorteio usa `std::mt19937` com seed passada ao `Game` (testes usam seed fixa).
- **`Game`** (`src/Game.h`) e o dono do estado: lista de niveis em sequencia, `Player`, inimigos, tiros, curas, `Hazards`, `WirePuzzle`, `Profile`. Modos: `Playing` -> `Puzzle` (mini-desafio ao abrir a porta com todas as ferramentas) -> proximo nivel ou `Complete`. `respawn()` volta ao inicio do bloco restaurando inimigos e mantendo ferramentas.
- **`GameRender`** so le o `Game` por getters const e desenha numa tela virtual de 256x224 (`kViewWidth`/`kViewHeight`), escalada por inteiro em `main.cpp`.
- **`Room`** e uma grade de tiles 16x16 em texto (`.` vazio, `#` solido, `H` escada, `^` espinho, `~` choque, `C` bloco que cai, `P` inicio). Os testes montam salas a partir de strings; fora dos limites conta como solido.
- **Niveis**: `assets/levels/cnnx.ldtk` (editor LDtk), lido por `LevelLoader` (`parseLevel`/`loadLevel` lancam `std::runtime_error` com mensagem clara). IDs `w<mundo>_f<fase>_b<bloco>`; a lista de niveis carregados esta hardcoded em `main.cpp`. Camadas, entidades e campos do nivel estao descritos no `README.md`.
- **Forca do bloco** (1-3) escala vida/dano/velocidade dos inimigos via `forceStats()` (`src/Enemy.h`), espelhando a tabela de `docs/design.md`.
- **`Profile`** guarda conquistas de vida maxima em `%APPDATA%\CnnxResgateDosFreelas\save.json`; `Profile()` sem caminho nao salva (usado nos testes).

## Testes

- `tests/test_game.cpp` usa um `Driver` que alimenta `Game::step` com entradas seguradas, e helpers como `arena(...)` e `solvePuzzle(...)`.
- `testBlockCanBeFinished("<id>")` roda um bot que completa o bloco real do `.ldtk` (caminho via `CNNX_LEVEL_PATH`). Mudancas no mapa ou na fisica do jogador podem quebrar esse teste.

## Convencoes

- Codigo, comentarios, docs e mensagens de teste em portugues, sem acentos.
- Ao adicionar algo que precise de arte, inclua em `docs/sprites.md`. O sprite do jogador e gerado por `python tools/gen_player_sprite.py assets/sprites/player.png`; a ordem das colunas tem que bater com `PlayerFrame` em `src/GameRender.cpp`.
- `README.md` esta em UTF-16 LE; preserve a codificacao ao editar.
