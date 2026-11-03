# Lista de sprites

Tudo no jogo ainda e desenhado com blocos coloridos. Esta lista guarda o que vai precisar de arte. Sempre que algo novo entrar no jogo, adicione aqui.

A tela tem 256x224 e os tiles tem 16x16 px. Os tamanhos abaixo sao as caixas de colisao atuais, e o desenho pode passar um pouco delas.

## Personagem

Caixa de 14x28 px, em quadros de 32x32 px. O protagonista e um cara de TI: meio gordinho, cabelo comprido, meio calvo, de oculos, com camiseta escura com `</>`. O tiro sai de um leitor de credencial na mao.

A primeira versao fica em `assets/sprites/player.png`, gerada por `python tools/gen_player_sprite.py assets/sprites/player.png`. Se a arte passar a ser feita em um editor, o PNG vira a fonte e o script deixa de ser usado. A ordem das colunas precisa continuar igual a de `PlayerFrame` em `src/GameRender.cpp`.

- [x] Parado
- [x] Correndo
- [x] Pulando e caindo
- [x] Dash no chao e no ar, com o mesmo quadro
- [x] Deslizando na parede
- [x] Pulo de parede, reaproveitando o quadro de pulo
- [x] Subindo escada
- [x] Tomando dano
- [x] Atirando em todas as poses
- [ ] Morrendo, hoje reaproveita o quadro de dano
- [ ] Efeito de carga media e carga total, hoje e so uma piscada de cor

## Tiros

- [ ] Tiro normal (6x4 px)
- [ ] Carga media (10x8 px)
- [ ] Carga total (16x12 px)
- [ ] Projetil dos inimigos (6x6 px)

## Inimigos

Cada inimigo precisa de tres evolucoes: pequeno (forca 1), medio (forca 2) e alto (forca 3). A cada forca, a caixa cresce 2 px em largura e altura. Os tamanhos abaixo sao os da forca 1.

- [ ] Rastejante (14x16 px): andando e virando
- [ ] Lancador (16x20 px): parado e atirando
- [ ] Voador passivo (14x12 px): voando
- [ ] Voador perseguidor (14x12 px): voando e atacando
- [ ] Voador atirador (14x12 px): voando e atirando
- [ ] Efeito de inimigo destruido

## Perigos e cenario

- [ ] Tiles solidos de cada fase, com versoes de Sao Paulo e Rio de Janeiro
- [ ] Escada
- [ ] Espinhos, em tile de 16x16 px
- [ ] Piso de choque, ligado e desligado
- [ ] Bloco de ponte que cai: parado, tremendo e caindo
- [ ] Objetos que caem (32x16 px): tapumes, pedacos de octanorme e outras coisas de evento
- [ ] Porta fechada e aberta (16x32 px)
- [ ] Fundo do evento vivo, com a montagem avancando a cada fase ate o dia da abertura
- [ ] Efeito da zona escura, se a versao em blocos de 4 px nao bastar

## Itens

- [ ] Cura pequena (6x6 px)
- [ ] Cura media (8x8 px)
- [ ] Cura grande (12x12 px)
- [ ] Conquista de vida maxima (16x16 px)

## Ferramentas

Todas em 16x16 px.

- [ ] Alicate de crimpar
- [ ] Cabo de rede
- [ ] Conector RJ45
- [ ] Chave de fenda
- [ ] Fio eletrico
- [ ] Tomada

## Mini-desafios

- [ ] RJ45: 8 fios, conector e testador de cabo
- [ ] Tomada: fios de fase, neutro e terra, e terminais F, N e T

## Interface

- [ ] Barra de vida em coluna, com bloco cheio e vazio
- [ ] Barra de especial das armas, ao lado da barra de vida
- [ ] Contador de ferramentas
- [ ] Avisos de porta trancada e vida maxima
- [ ] Tela de bloco concluido

## Personagens e progressao futuros

- [ ] Bosses de cada fase, incluindo o ataque especial
- [ ] Boss final
- [ ] Freelas resgatados
- [ ] Catracas
- [ ] Montagem da catraca com o freela, na fase final
