---
name: pixel-art
description: Criar ou ajustar arte em pixel art do cnnx-the-game (sprites, telas, cenario, efeitos). Use sempre que for gerar ou mudar um PNG em assets/sprites, um gerador em tools/gen_*.py, ou um efeito desenhado em src/GameRender.cpp.
---

# Pixel art no cnnx-the-game

A arte nasce de codigo: cada PNG em `assets/sprites/` sai de um gerador em `tools/gen_*.py` (Python so com a biblioteca padrao, reaproveitando `write_png` de `tools/gen_player_sprite.py`). Efeitos que se mexem (fachos de luz, piscadas, tremidas) ficam em `src/GameRender.cpp`, desenhados por cima ou entre camadas de PNG.

## Ciclo de trabalho

Baseado em "How to make good pixel art effects with Claude" (Sprite Fusion): codigo e massa de modelar, e o olho decide.

1. Leia `docs/sprites.md` para o tamanho da caixa e o que ja existe. A tela e 256x224 e o tile e 16x16.
2. Escreva ou ajuste o gerador. Quando a ideia ainda esta solta, faca 2 ou 3 variacoes (cores, formas) em vez de uma so.
3. Gere uma previa ampliada no scratchpad (os geradores aceitam um caminho extra para a previa) e abra a imagem com Read para olhar de verdade.
4. Corrija o que nao funciona olhando a previa: leitura da silhueta, contraste com o fundo, nomes legiveis. Repita ate ficar bom, nao entregue o primeiro rascunho.
5. Para efeitos animados, rode o jogo e capture a janela (`Start-Process .\build\cnnx.exe`, `CopyFromScreen` do System.Drawing) para conferir no contexto real.
6. Gere os PNGs finais em `assets/sprites/`, rode `cmake --build build` (copia os assets) e marque o item em `docs/sprites.md`.

## Regras de estilo

- Paleta curta por objeto: 3 ou 4 tons (luz, base, sombra, contorno). Defina as cores como constantes no topo do gerador.
- Luz vindo de cima e da esquerda: brilho no topo e na borda esquerda, sombra embaixo e na direita.
- Contorno de 1 px em roxo bem escuro `(14, 8, 24)` ou `(12, 12, 18)`, nunca preto puro. Silhuetas de fundo (publico, cidade) ficam sem contorno.
- Degrade so em faixas com pontilhado de xadrez na transicao, nunca degrade liso.
- Pixels inteiros: nada de anti-aliasing, alfa so 0 ou 255 nos PNGs. Transparencia parcial so em efeitos do jogo (fachos, escuridao).
- Texto pequeno na arte usa a fonte 3x5 de `tools/gen_title_screen.py` (so maiusculas, sem acento). Texto de interface usa `DrawText` com tamanho 10 e sombra de 1 px.
- O desenho pode passar um pouco da caixa de colisao, mas a base (pes, chao) precisa bater com ela.

## Tema

O jogo se passa em festivais e no credenciamento de eventos (`docs/design.md`). Os mundos sao festivais com nomes trocados: Rock in Rede, Lollaparafuso, Planeta Catraca e Rock the Morro. Lembre a identidade visual do festival real (cores, formato do portal) sem copiar logo nem nome. Elementos de trabalho sao bem-vindos na arte: catracas, credenciais, cabos, ferramentas, tapumes.
