"""Gera assets/sprites/turnstile.png: a catraquinha da explosao de morte do protagonista.

Uma catraca tripe de 14x14 em 4 quadros, cada um girado 90 graus (giro sem perda de pixel).
O movimento das catraquinhas e feito em drawDeathBurst, em src/GameRender.cpp.

Uso: python tools/gen_death_effect.py assets/sprites/turnstile.png [previa.png]
A previa mostra o movimento em quadros (contados desde a morte), so para conferir.
"""
import math
import sys

from gen_player_sprite import write_png

SIZE = 14

OUTLINE = (14, 8, 24)
STEEL_LIGHT = (214, 218, 232)
STEEL = (150, 156, 172)
STEEL_SHADE = (96, 100, 120)
ARM = (240, 242, 252)
HUB = (70, 74, 92)
LIGHT_ON = (90, 230, 120)


def base_frame():
    pixels = {}

    def rect(x0, y0, x1, y1, color):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                pixels[(x, y)] = color

    # Gabinete com tampa clara, luz verde e sombra a direita
    rect(2, 3, 6, 11, STEEL)
    rect(2, 3, 2, 11, STEEL_LIGHT)
    rect(2, 3, 6, 3, STEEL_LIGHT)
    rect(6, 4, 6, 11, STEEL_SHADE)
    rect(1, 11, 7, 11, STEEL_SHADE)
    pixels[(4, 5)] = LIGHT_ON
    # Tres bracos saindo do eixo
    for x, y in ((8, 6), (9, 6), (10, 6), (11, 6), (8, 5), (9, 4), (10, 3), (8, 7), (9, 8), (10, 9)):
        pixels[(x, y)] = ARM
    pixels[(7, 6)] = HUB
    pixels[(7, 5)] = HUB

    filled = set(pixels)
    for x, y in filled:
        for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if (nx, ny) not in filled and 0 <= nx < SIZE and 0 <= ny < SIZE:
                pixels[(nx, ny)] = OUTLINE
    return pixels


def rotated(pixels, turns):
    result = pixels
    for _ in range(turns):
        result = {(SIZE - 1 - y, x): color for (x, y), color in result.items()}
    return result


FRAMES = [rotated(base_frame(), turns) for turns in range(4)]


def write_sheet(path):
    rows = [[(0, 0, 0, 0)] * (SIZE * len(FRAMES)) for _ in range(SIZE)]
    for index, frame in enumerate(FRAMES):
        for (x, y), color in frame.items():
            rows[y][index * SIZE + x] = color + (255,)
    write_png(path, SIZE * len(FRAMES), SIZE, rows)


# ------------------------------------------------- previa do movimento

W, H = 256, 224
BURST_FRAME = 15
SKY = (24, 28, 48)
FLOOR = (62, 92, 130)
FLOOR_Y = 160
ORIGIN = (128, 140)


# Mesmos numeros de drawDeathBurst em src/GameRender.cpp: dois aneis de 8, o de fora
# defasado meio passo e mais rapido, com gravidade leve para as pecas cairem em arco.
RING_SPEEDS = (1.8, 2.9)
GRAVITY = 0.045


def burst_motion(t):
    out = []
    for ring, speed in enumerate(RING_SPEEDS):
        for i in range(8):
            angle = (i + ring * 0.5) * math.pi / 4
            dx = math.cos(angle) * speed * t
            dy = math.sin(angle) * speed * t + GRAVITY * t * t / 2
            out.append((dx, dy, i + ring))
    return out


VARIATIONS = [burst_motion]
PREVIEW_TIMES = [8, 16, 20, 28, 40, 56, 74]


def draw_cell(canvas, cell_x, cell_y, motion, t):
    for y in range(H):
        for x in range(W):
            canvas[cell_y + y][cell_x + x] = FLOOR if y >= FLOOR_Y else SKY
    if t < BURST_FRAME:
        # Antes de explodir o personagem fica parado piscando; aqui so a caixa dele
        color = (255, 255, 255) if t % 4 < 2 else (255, 90, 90)
        for y in range(ORIGIN[1] - 14, ORIGIN[1] + 14):
            for x in range(ORIGIN[0] - 7, ORIGIN[0] + 7):
                canvas[cell_y + y][cell_x + x] = color
        return
    if t < BURST_FRAME + 6:
        radius = 3 + (t - BURST_FRAME) * 4
        for y in range(H):
            for x in range(W):
                d = math.hypot(x - ORIGIN[0], y - ORIGIN[1])
                if radius - 1.5 <= d <= radius:
                    canvas[cell_y + y][cell_x + x] = (255, 250, 210)
    for dx, dy, index in motion(t - BURST_FRAME):
        frame = FRAMES[(t // 4 + index) % 4]
        left = round(ORIGIN[0] + dx) - SIZE // 2
        top = round(ORIGIN[1] + dy) - SIZE // 2
        for (x, y), color in frame.items():
            px, py = left + x, top + y
            if 0 <= px < W and 0 <= py < H:
                canvas[cell_y + py][cell_x + px] = color


def write_preview(path, scale=2):
    width = W * len(PREVIEW_TIMES)
    height = H * len(VARIATIONS)
    canvas = [[(0, 0, 0)] * width for _ in range(height)]
    for row, motion in enumerate(VARIATIONS):
        for column, t in enumerate(PREVIEW_TIMES):
            draw_cell(canvas, column * W, row * H, motion, t)
    # Linha separando as celulas
    for y in range(height):
        for x in range(width):
            if x % W == 0 or y % H == 0:
                canvas[y][x] = (255, 255, 255)
    rows = []
    for row in canvas:
        scaled = [color + (255,) for color in row for _ in range(scale)]
        rows.extend([scaled] * scale)
    write_png(path, width * scale, height * scale, rows)


if __name__ == "__main__":
    write_sheet(sys.argv[1])
    if len(sys.argv) > 2:
        write_preview(sys.argv[2], scale=1)
