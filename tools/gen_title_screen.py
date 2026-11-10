"""Gera a arte da tela inicial em duas camadas de 256x224.

title_bg.png: ceu, estrelas, lua, morros e cidade (opaca).
title_fg.png: logo, faixa do subtitulo, portais dos festivais e publico (com transparencia).
Os fachos de luz e as estrelas que piscam sao desenhados no jogo, entre as duas camadas.

Os quatro portais sao os mundos do jogo, inspirados em festivais reais com nomes trocados:
Rock in Rede, Lollaparafuso, Planeta Catraca e Rock the Morro.

Uso: python tools/gen_title_screen.py assets/sprites [previa.png]
A previa junta as camadas em escala 3, so para conferir a arte.
"""
import math
import os
import random
import sys

from gen_player_sprite import write_png

W, H = 256, 224
GROUND_Y = 192
OUTLINE = (14, 8, 24)

SKY = [
    (12, 8, 28),
    (18, 12, 40),
    (28, 16, 56),
    (42, 20, 72),
    (62, 24, 88),
    (90, 30, 100),
    (124, 40, 108),
    (166, 54, 110),
    (204, 78, 104),
]


class Layer:
    def __init__(self, fill=None):
        self.px = [[fill] * W for _ in range(H)]

    def set(self, x, y, color):
        if 0 <= x < W and 0 <= y < H:
            self.px[y][x] = color

    def get(self, x, y):
        if 0 <= x < W and 0 <= y < H:
            return self.px[y][x]
        return None

    def rect(self, x0, y0, x1, y1, color):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.set(x, y, color)

    def disc(self, cx, cy, r, color):
        for y in range(cy - r, cy + r + 1):
            for x in range(cx - r, cx + r + 1):
                if (x - cx) ** 2 + (y - cy) ** 2 <= r * r + r:
                    self.set(x, y, color)

    def line(self, x0, y0, x1, y1, width, color):
        steps = max(abs(x1 - x0), abs(y1 - y0), 1)
        for step in range(steps + 1):
            x = round(x0 + (x1 - x0) * step / steps)
            y = round(y0 + (y1 - y0) * step / steps)
            self.rect(x, y, x + width - 1, y + width - 1, color)

    def outline(self, color=OUTLINE):
        filled = {(x, y) for y in range(H) for x in range(W) if self.px[y][x] is not None}
        for x, y in filled:
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if (nx, ny) not in filled:
                    self.set(nx, ny, color)

    def stamp(self, other):
        for y in range(H):
            for x in range(W):
                if other.px[y][x] is not None:
                    self.px[y][x] = other.px[y][x]

    def rgba_rows(self):
        return [[(c + (255,)) if c else (0, 0, 0, 0) for c in row] for row in self.px]


# Fonte 3x5 para os nomes dos portais
SMALL_FONT = {
    "A": [".X.", "X.X", "XXX", "X.X", "X.X"],
    "C": [".XX", "X..", "X..", "X..", ".XX"],
    "D": ["XX.", "X.X", "X.X", "X.X", "XX."],
    "E": ["XXX", "X..", "XX.", "X..", "XXX"],
    "F": ["XXX", "X..", "XX.", "X..", "X.."],
    "H": ["X.X", "X.X", "XXX", "X.X", "X.X"],
    "I": ["XXX", ".X.", ".X.", ".X.", "XXX"],
    "K": ["X.X", "X.X", "XX.", "X.X", "X.X"],
    "L": ["X..", "X..", "X..", "X..", "XXX"],
    "M": ["X.X", "XXX", "XXX", "X.X", "X.X"],
    "N": ["XX.", "X.X", "X.X", "X.X", "X.X"],
    "O": [".X.", "X.X", "X.X", "X.X", ".X."],
    "P": ["XX.", "X.X", "XX.", "X..", "X.."],
    "R": ["XX.", "X.X", "XX.", "X.X", "X.X"],
    "S": [".XX", "X..", ".X.", "..X", "XX."],
    "T": ["XXX", ".X.", ".X.", ".X.", ".X."],
    "U": ["X.X", "X.X", "X.X", "X.X", "XXX"],
    " ": ["...", "...", "...", "...", "..."],
}

# Fonte 5x7 do logo, ampliada em blocos
BIG_FONT = {
    "C": [".XXX.", "X...X", "X....", "X....", "X....", "X...X", ".XXX."],
    "N": ["X...X", "XX..X", "XX..X", "X.X.X", "X..XX", "X..XX", "X...X"],
    "X": ["X...X", "X...X", ".X.X.", "..X..", ".X.X.", "X...X", "X...X"],
}


def text_width(text):
    return len(text) * 4 - 1


def draw_text(layer, text, x, y, color):
    for i, char in enumerate(text):
        for gy, row in enumerate(SMALL_FONT[char]):
            for gx, cell in enumerate(row):
                if cell == "X":
                    layer.set(x + i * 4 + gx, y + gy, color)


def draw_text_centered(layer, text, cx, y, color):
    draw_text(layer, text, cx - text_width(text) // 2, y, color)


# ---------------------------------------------------------------- fundo


def draw_sky(bg):
    band = 15
    for y in range(H):
        position = y / band
        index = min(int(position), len(SKY) - 1)
        frac = position - int(position)
        for x in range(W):
            color = SKY[index]
            # Pontilhado nas ultimas linhas de cada faixa, sem degrade liso
            if index + 1 < len(SKY):
                if frac > 0.86 and (x + y) % 2 == 0:
                    color = SKY[index + 1]
                elif frac > 0.73 and x % 2 == 0 and y % 2 == 0:
                    color = SKY[index + 1]
            bg.set(x, y, color)


def draw_stars(bg):
    rng = random.Random(3)
    for _ in range(80):
        x = rng.randrange(W)
        y = rng.randrange(96)
        bright = rng.random()
        if bright > 0.92:
            bg.set(x, y, (255, 255, 255))
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                bg.set(nx, ny, (150, 140, 210))
        elif bright > 0.6:
            bg.set(x, y, (210, 200, 255))
        else:
            bg.set(x, y, (110, 96, 170))


def draw_moon(bg):
    cx, cy, r = 228, 24, 11
    for y in range(cy - r, cy + r + 1):
        for x in range(cx - r, cx + r + 1):
            inside = (x - cx) ** 2 + (y - cy) ** 2 <= r * r + r
            bite = (x - cx - 5) ** 2 + (y - cy + 4) ** 2 <= (r - 1) ** 2
            if inside and not bite:
                edge = (x - cx - 5) ** 2 + (y - cy + 4) ** 2 <= (r + 1) ** 2
                bg.set(x, y, (214, 196, 150) if edge else (252, 242, 204))


def ridge_far(x):
    return 126 - 10 * math.sin(x * 0.021 + 0.5) - 5 * math.sin(x * 0.063 + 1.7) - 2 * math.sin(x * 0.17 + 0.3)


def hump(x, cx, half, height):
    t = (x - cx) / half
    return height * math.sqrt(1 - t * t) if abs(t) < 1 else 0.0


# Morros arredondados do Rio, longe da cidade no meio
HUMPS = [(18, 16, 46), (48, 26, 26), (96, 18, 12), (232, 30, 38), (206, 16, 16)]


def ridge_near(x):
    return 160 - max(hump(x, cx, half, height) for cx, half, height in HUMPS)


def draw_mountains(bg):
    for x in range(W):
        top = int(ridge_far(x))
        for y in range(top, H):
            bg.set(x, y, (70, 30, 92))
        bg.set(x, top, (112, 52, 124))


def draw_city(bg):
    rng = random.Random(11)
    x = 112
    tallest = (0, 999)
    while x < 214:
        width = rng.randrange(6, 13)
        top = 158 - rng.randrange(10, 34)
        for bx in range(x, x + width):
            for y in range(top, H):
                bg.set(bx, y, (34, 18, 52))
        for wy in range(top + 2, 156, 3):
            for wx in range(x + 1, x + width - 1, 2):
                if rng.random() < 0.28:
                    bg.set(wx, wy, (250, 200, 110))
        if top < tallest[1]:
            tallest = (x + width // 2, top)
        x += width + rng.randrange(0, 3)
    ax, ay = tallest
    bg.rect(ax, ay - 6, ax, ay - 1, (34, 18, 52))
    bg.set(ax, ay - 7, (255, 70, 70))


def draw_near_hills(bg):
    for x in range(W):
        top = int(ridge_near(x))
        if top >= 160:
            continue
        for y in range(top, H):
            bg.set(x, y, (40, 20, 62))
        bg.set(x, top, (84, 44, 106))


def draw_ground(bg):
    for y in range(160, H):
        for x in range(W):
            color = (24, 14, 36) if y > 163 or (x + y) % 2 == 0 else (40, 20, 62)
            bg.set(x, y, color)


def build_background():
    bg = Layer()
    draw_sky(bg)
    draw_stars(bg)
    draw_moon(bg)
    draw_mountains(bg)
    draw_city(bg)
    draw_near_hills(bg)
    draw_ground(bg)
    return bg


# ---------------------------------------------------------------- frente

LOGO_ROWS = [
    (255, 242, 150),
    (255, 222, 84),
    (255, 194, 62),
    (255, 162, 52),
    (250, 126, 52),
    (236, 92, 58),
    (212, 62, 74),
]


def draw_logo(fg):
    scale, gap, text = 4, 4, "CNNX"
    width = len(text) * 5 * scale + (len(text) - 1) * gap
    x0, y0 = (W - width) // 2 - 1, 6
    letters = {}
    for i, char in enumerate(text):
        for gy, row in enumerate(BIG_FONT[char]):
            for gx, cell in enumerate(row):
                if cell != "X":
                    continue
                for sy in range(scale):
                    for sx in range(scale):
                        x = x0 + i * (5 * scale + gap) + gx * scale + sx
                        letters[(x, y0 + gy * scale + sy)] = LOGO_ROWS[gy]

    extrude = {}
    for depth in range(1, 4):
        for x, y in letters:
            if (x + depth, y + depth) not in letters:
                extrude[(x + depth, y + depth)] = (120, 26, 86) if depth < 3 else (84, 18, 70)

    logo = Layer()
    for (x, y), color in extrude.items():
        logo.set(x, y, color)
    for (x, y), color in letters.items():
        # Brilho no topo de cada bloco e sombra na borda direita
        if (x, y - 1) not in letters:
            color = (255, 252, 220)
        elif (x + 1, y) not in letters:
            color = tuple(max(c - 40, 0) for c in color)
        logo.set(x, y, color)
    logo.outline()
    fg.stamp(logo)


def draw_ribbon(fg):
    ribbon = Layer()
    left, right, top, bottom = 58, 197, 47, 59
    for x in range(left, right + 1):
        for y in range(top, bottom + 1):
            ribbon.set(x, y, (28, 140, 156) if y > top else (90, 210, 210))
    ribbon.rect(left, bottom, right, bottom, (18, 96, 116))
    # Pontas dobradas da faixa
    for side, direction in ((left, -1), (right, 1)):
        for i in range(8):
            x = side + direction * (i + 1)
            for y in range(top + 3, bottom + 4):
                notch = abs(y - (top + bottom + 6) // 2) < i - 4
                if not notch:
                    ribbon.set(x, y, (18, 96, 116))
    ribbon.outline()
    fg.stamp(ribbon)


def draw_turnstile(layer, x):
    layer.rect(x, 182, x + 2, GROUND_Y - 1, (150, 156, 172))
    layer.rect(x, 182, x, GROUND_Y - 1, (196, 200, 214))
    layer.rect(x - 1, 180, x + 3, 181, (90, 94, 112))
    layer.rect(x + 3, 185, x + 6, 185, (196, 200, 214))
    layer.set(x + 1, 183, (90, 230, 120))


def draw_turnstiles(layer, cx):
    draw_turnstile(layer, cx - 13)
    draw_turnstile(layer, cx + 7)


def portal_rock_in_rede(x0):
    p = Layer()
    cx = x0 + 28
    red, red_light, red_dark = (200, 36, 52), (236, 84, 88), (132, 20, 42)
    for x in (x0, x0 + 48):
        p.rect(x, 140, x + 7, GROUND_Y - 1, red)
        p.rect(x + 1, 140, x + 1, GROUND_Y - 1, red_light)
        p.rect(x + 7, 140, x + 7, GROUND_Y - 1, red_dark)
        p.rect(x + 4, 156, x + 4, GROUND_Y - 4, (245, 236, 232))
    for y in range(110, 141):
        for x in range(x0, x0 + 56):
            d = math.hypot(x - cx + 0.5, y - 140)
            if 20 < d <= 28:
                p.set(x, y, red_dark if d > 26.5 else red)
            elif 19 < d <= 20:
                p.set(x, y, (245, 236, 232))
    # Luzes ao longo do arco
    for step in range(9):
        angle = math.pi * step / 8
        p.set(round(cx - 0.5 + 24 * math.cos(angle)), round(140 - 24 * math.sin(angle)), (255, 248, 200))
    # Globo de rede com braco de guitarra
    p.line(cx + 4, 125, cx + 13, 114, 2, (120, 70, 40))
    p.rect(cx + 12, 111, cx + 15, 114, (60, 36, 24))
    p.disc(cx, 130, 8, (44, 96, 208))
    for y in range(122, 139):
        for x in range(cx - 8, cx + 9):
            if p.get(x, y) == (44, 96, 208) and ((x - cx) % 5 == 0 or (y - 130) % 5 == 0):
                p.set(x, y, (110, 170, 255))
    for nx, ny in ((cx - 4, 126), (cx + 3, 128), (cx - 2, 134), (cx + 5, 133)):
        p.set(nx, ny, (255, 255, 255))
    p.rect(x0, 141, x0 + 55, 154, (245, 236, 232))
    p.rect(x0, 154, x0 + 55, 154, (190, 176, 176))
    draw_text_centered(p, "ROCK IN", cx, 142, (180, 20, 40))
    draw_text_centered(p, "REDE", cx, 148, (180, 20, 40))
    draw_turnstiles(p, cx)
    p.outline()
    return p


def portal_lollaparafuso(x0):
    p = Layer()
    cx = x0 + 28
    stripes = [(250, 80, 160), (255, 210, 60), (60, 210, 230), (120, 230, 110)]
    for left in (x0 + 2, x0 + 46):
        for y in range(132, GROUND_Y):
            for x in range(left, left + 8):
                p.set(x, y, stripes[((x + y) // 3) % 4])
    p.rect(x0, 114, x0 + 55, 131, (240, 70, 150))
    for x in range(x0, x0 + 56):
        p.set(x, 114, (255, 220, 70))
        p.set(x, 131, (255, 220, 70))
        if (x - x0) % 8 in (2, 3, 4, 5):
            p.set(x, 132, (255, 220, 70))
        if (x - x0) % 8 in (3, 4):
            p.set(x, 133, (255, 220, 70))
    p.rect(x0, 114, x0, 131, (255, 220, 70))
    p.rect(x0 + 55, 114, x0 + 55, 131, (255, 220, 70))
    draw_text_centered(p, "LOLLA", cx, 117, (255, 255, 255))
    draw_text_centered(p, "PARAFUSO", cx, 124, (255, 228, 90))
    # Parafusos em cima do letreiro
    for sx in (x0 + 6, x0 + 49):
        p.rect(sx - 1, 111, sx + 1, 113, (150, 156, 172))
        p.disc(sx, 109, 2, (196, 200, 214))
        p.rect(sx - 1, 109, sx + 1, 109, (90, 94, 112))
    draw_turnstiles(p, cx)
    p.outline()
    return p


def portal_planeta_catraca(x0):
    p = Layer()
    cx = x0 + 28
    blue, blue_light, wave = (40, 100, 200), (80, 150, 236), (100, 214, 244)
    for left in (x0 + 3, x0 + 45):
        for y in range(146, GROUND_Y):
            for x in range(left, left + 8):
                color = blue_light if x == left + 1 else blue
                if (y + round(1.5 * math.sin(x * 1.3))) % 6 == 0:
                    color = wave
                p.set(x, y, color)

    def ring(front):
        for y in range(116, 130):
            for x in range(cx - 19, cx + 20):
                dy = y - 123 + (x - cx) * 0.18
                value = ((x - cx) / 18) ** 2 + (dy / 4) ** 2
                if abs(value - 1) < 0.28 and (dy > 0) == front:
                    p.set(x, y, (232, 240, 255) if value < 1 else (150, 190, 230))

    ring(front=False)
    for y in range(112, 132):
        for x in range(cx - 10, cx + 11):
            d = math.hypot(x - cx, y - 122)
            if d <= 9.5:
                light = (x - cx) + (y - 122)
                color = (255, 196, 96) if light < -6 else (190, 80, 52) if light > 6 else (242, 132, 52)
                p.set(x, y, color)
    ring(front=True)
    p.rect(x0, 133, x0 + 55, 147, (26, 50, 120))
    for x in range(x0, x0 + 56):
        p.set(x, 133, wave)
        p.set(x, 147, wave)
    draw_text_centered(p, "PLANETA", cx, 135, (255, 255, 255))
    draw_text_centered(p, "CATRACA", cx, 141, (255, 176, 76))
    draw_turnstiles(p, cx)
    p.outline()
    return p


def portal_rock_the_morro(x0):
    p = Layer()
    cx = x0 + 28
    wood, wood_light, wood_dark = (130, 82, 44), (172, 114, 62), (88, 54, 30)
    for left in (x0 + 2, x0 + 46):
        p.rect(left, 136, left + 7, GROUND_Y - 1, wood)
        p.rect(left + 1, 136, left + 2, GROUND_Y - 1, wood_light)
        p.rect(left + 7, 136, left + 7, GROUND_Y - 1, wood_dark)
        for y in range(141, GROUND_Y, 9):
            p.rect(left, y, left + 7, y, wood_dark)
    for y in range(112, 137):
        for x in range(x0, x0 + 56):
            edge = 112 + abs(x - cx + 0.5) * 24 / 28
            if y < edge:
                continue
            if y - edge < 2.2:
                color = wood_light
            elif y < 119:
                color = (240, 245, 255)
            elif y < 125:
                color = (126, 124, 148) if (x + y) % 5 else (100, 98, 124)
            else:
                color = (62, 140, 72) if (x * 3 + y) % 7 else (44, 108, 56)
            p.set(x, y, color)
    p.rect(x0, 136, x0 + 55, 149, (110, 70, 38))
    for x in range(x0, x0 + 56):
        p.set(x, 136, wood_light)
        p.set(x, 149, wood_dark)
    draw_text_centered(p, "ROCK THE", cx, 138, (250, 236, 200))
    draw_text_centered(p, "MORRO", cx, 144, (194, 232, 112))
    draw_turnstiles(p, cx)
    p.outline()
    return p


PORTALS = [portal_rock_in_rede, portal_lollaparafuso, portal_planeta_catraca, portal_rock_the_morro]
PORTAL_SPACING = 62
PORTAL_FIRST_X = 7


def draw_crowd(fg):
    rng = random.Random(5)
    for row_y, radius, body, color, light in (
        (197, 3, 9, (44, 24, 62), (70, 40, 92)),
        (206, 4, 11, (18, 10, 28), (34, 20, 48)),
    ):
        x = rng.randrange(-4, 3)
        while x < W + 6:
            head_y = row_y + rng.randrange(-2, 3)
            fg.rect(x - body // 2, head_y + radius, x + body // 2, H - 1, color)
            fg.disc(x, head_y, radius, color)
            fg.set(x, head_y - radius, light)
            if rng.random() < 0.3:
                side = rng.choice((-1, 1))
                hand_x, hand_y = x + side * (body // 2 + 2), head_y - 9
                fg.line(x + side * (body // 2 - 1), head_y + radius + 1, hand_x, hand_y, 2, color)
                if rng.random() < 0.6:
                    fg.rect(hand_x, hand_y - 2, hand_x + 1, hand_y - 1, (255, 248, 210))
            x += body - 2 + rng.randrange(0, 4)


def build_foreground():
    fg = Layer()
    draw_logo(fg)
    draw_ribbon(fg)
    for index, portal in enumerate(PORTALS):
        fg.stamp(portal(PORTAL_FIRST_X + index * PORTAL_SPACING))
    draw_crowd(fg)
    return fg


def write_layer(path, layer):
    write_png(path, W, H, layer.rgba_rows())


def write_preview(path, bg, fg, scale=3):
    rows = []
    for y in range(H):
        row = []
        for x in range(W):
            color = fg.px[y][x] or bg.px[y][x]
            row.extend([color + (255,)] * scale)
        rows.extend([row] * scale)
    write_png(path, W * scale, H * scale, rows)


if __name__ == "__main__":
    out_dir = sys.argv[1]
    background = build_background()
    foreground = build_foreground()
    write_layer(os.path.join(out_dir, "title_bg.png"), background)
    write_layer(os.path.join(out_dir, "title_fg.png"), foreground)
    if len(sys.argv) > 2:
        write_preview(sys.argv[2], background, foreground)
