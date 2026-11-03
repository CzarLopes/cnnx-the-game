# Design do jogo

## Tema

O jogo se passa no dia a dia de controle de acesso e credenciamento de eventos. Elementos do trabalho viram desafios, bonus e melhorias. Os mundos sao os festivais, e as fases usam elementos das cidades, como Sao Paulo e Rio de Janeiro.

## Estrutura

- Cada mundo tem 6 fases.
- Cada fase tem 3 blocos de desafio e 1 bloco final com boss.
- Ao passar as 6 fases, a final do mundo traz elementos de todas as fases, os bosses com 110% da forca e depois um boss final.
- Identificador dos niveis: `w<mundo>_f<fase>_b<bloco>`, por exemplo `w1_f1_b1`.

## Blocos

- A forca dos inimigos sobe por bloco: 1 (pequeno), 2 (medio) e 3 (alto).
- Toda fase tem um inimigo rastejante, um voador e um lancador de projeteis.
- Variaveis de desafio: escadas, espinhos, paredes, blocos caindo, locais com choque e locais escuros.
- Para sair do bloco, o jogador junta ferramentas e faz um mini-desafio que abre a porta.
- Ideias de mini-desafio: montar RJ45, keystone ou tomada, e juntar mesas e cadeiras. Outros entram durante o desenvolvimento.
- Ao morrer, o jogador volta ao inicio do bloco, com inimigos restaurados e ferramentas mantidas.

## Valores iniciais

Estes numeros sao um ponto de partida e devem ser calibrados jogando.

| Forca | Vida | Dano de contato | Dano do projetil | Velocidade |
|---|---|---|---|---|
| 1 | 2 | 2 | 2 | 100% |
| 2 | 4 | 3 | 3 | 115% |
| 3 | 7 | 4 | 4 | 130% |

- Jogador: 16 de vida. Tiro normal causa 1, carga media 2 e carga total 4.
- Espinho: 4 de dano. Choque: 3 de dano quando ligado.
- Primeiro bloco: 9 inimigos em cerca de 6 telas, sendo 4 rastejantes, 3 voadores e 2 lancadores.
