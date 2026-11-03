# Design do jogo

Nome: **Cnnx em o Resgate dos Freelas**. "Freelas" e a forma carinhosa de chamar os freelancers que trabalham com a gente.

## Tema

O jogo se passa no dia a dia de controle de acesso e credenciamento de eventos. Elementos do trabalho viram desafios, bonus e melhorias. Os mundos sao os festivais, e as fases usam elementos das cidades, como Sao Paulo e Rio de Janeiro.

## Objetivo e progressao

- O grande objetivo do jogo e resgatar os freelas e juntar as catracas.
- Ao derrotar o boss de uma fase, o jogador resgata um freela e leva uma catraca junto.
- O fundo da tela e o evento vivo: a montagem acontece enquanto o jogador avanca.
- Cada fase concluida deixa o evento mais perto do dia da abertura, quando o acesso acontece.
- Objetos que caem sao coisas da montagem, como tapumes e pedacos de octanorme.

## Estrutura

- Cada mundo tem 6 fases.
- Cada fase tem 3 blocos de desafio e 1 bloco final com boss.
- A fase final do mundo so abre com todos os freelas resgatados e todas as catracas.
- Na fase final, cada boss e reenfrentado nesta ordem:
  1. Passar por um bloco que resume os tres blocos da fase daquele boss.
  2. Ajudar o freela a montar a catraca, o que abre o bloco do boss.
  3. Enfrentar o boss com 110% da forca.
- Depois de todos os reenfrentamentos vem o boss final.
- Identificador dos niveis: `w<mundo>_f<fase>_b<bloco>`, por exemplo `w1_f1_b1`.

## Blocos

- A forca dos inimigos sobe por bloco: 1 (pequeno), 2 (medio) e 3 (alto).
- Toda fase tem inimigos rastejantes e lancadores de projeteis.
- Todo bloco tem os tres voadores: passivo, que so voa e atrapalha; perseguidor, que vem para cima; e atirador, que mira no jogador.
- Variaveis de desafio: escadas, espinhos, paredes, blocos caindo, locais com choque e locais escuros.
- Blocos caindo tem duas formas: ponte que desaba depois de pisar e objeto suspenso que despenca quando o jogador se aproxima.
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
- Espinho: 4 de dano. Choque: 3 de dano quando ligado. Objeto caindo: 3 de dano.
- Primeiro bloco: 18 inimigos em cerca de 7 telas, sendo 6 rastejantes, 4 lancadores, 3 voadores passivos, 3 perseguidores e 2 atiradores.
