"""Gera a arte do mapa de mundos e da tela de fases.

world_map.png: oceano e continente onde ficam os festivais (256x224, opaco).
world_icons.png: um icone de 32x32 por mundo, na ordem de worlds() em src/Worlds.cpp:
    Planeta Catraca, Rock in Rede, Lollaparafuso, Rock the Morro.
phases_w1.png: praia do Planeta Catraca, fundo da tela de fases (256x224, opaco).

As posicoes dos icones, os caminhos e os pontos de fase sao desenhados em
src/GameRender.cpp, entao estes PNGs so tem o cenario.

Uso: python tools/gen_world_screens.py assets/sprites [pasta_da_previa]
"""
import math
import os
import random
import sys

from gen_player_sprite import write_png
from gen_title_screen import OUTLINE, Layer, portal_planeta_catraca

W, H = 256, 224
ICON = 32

DEEP_SEA = (20, 46, 104)
SEA = (30, 70, 142)
SHALLOW = (52, 116, 178)
WAVE = (110, 176, 226)
SAND = (232, 208, 146)
SAND_SHADE = (204, 172, 110)
GRASS = (78, 156, 84)
GRASS_DARK = (56, 124, 70)
FOREST = (36, 96, 58)
ROCK = (128, 126, 150)
ROCK_SHADE = (96, 94, 120)
SNOW = (240, 245, 255)
ROOF = (196, 92, 70)
WALL = (232, 220, 204)


# ---------------------------------------------------------------- mapa

BLOBS = [(128, 108, 98, 64), (78, 136, 48, 40), (196, 94, 46, 44), (150, 160, 52, 30), (112, 60, 58, 34), (210, 138, 30, 26),
         (30, 46, 10, 7), (236, 186, 12, 8)]


def land_value(x, y):
    value = -10.0
    for cx, cy, rx, ry in BLOBS:
        d = ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2
        value = max(value, 1.0 - d)
    # Recorte da costa
    return value + 0.06 * math.sin(x * 0.21) + 0.05 * math.sin(y * 0.27 + x * 0.05)


def is_land(x, y):
    return land_value(x, y) > 0.05


def distance_to_coast(x, y, limit):
    for d in range(1, limit + 1):
        for nx, ny in ((x + d, y), (x - d, y), (x, y + d), (x, y - d)):
            if is_land(nx, ny) != is_land(x, y):
                return d
    return limit + 1


def draw_ocean(m):
    for y in range(H):
        for x in range(W):
            d = distance_to_coast(x, y, 6)
            if d <= 2:
                color = SHALLOW
            elif d <= 5:
                color = SHALLOW if (x + y) % 2 == 0 else SEA
            else:
                color = SEA if (x // 2 + y // 2) % 9 else DEEP_SEA
            m.set(x, y, color)
    rng = random.Random(4)
    for _ in range(70):
        x, y = rng.randrange(W), rng.randrange(H)
        if distance_to_coast(x, y, 8) > 8 and not is_land(x, y):
            m.rect(x, y, x + 2, y, WAVE)


def draw_land(m):
    for y in range(H):
        for x in range(W):
            if not is_land(x, y):
                continue
            d = distance_to_coast(x, y, 3)
            if d <= 2:
                color = SAND if d == 1 or (x + y) % 2 else SAND_SHADE
            else:
                # Grama lisa com pontinhos escuros espalhados
                speck = (x * 73856093 ^ y * 19349663) % 13 == 0
                color = GRASS_DARK if speck or (d == 3 and (x + y) % 2) else GRASS
            m.set(x, y, color)


def draw_tree(m, x, y):
    m.rect(x, y + 3, x, y + 4, (92, 60, 36))
    m.rect(x - 1, y, x + 1, y + 2, FOREST)
    m.set(x, y - 1, FOREST)
    m.set(x - 1, y, GRASS_DARK)


def draw_forest(m):
    rng = random.Random(8)
    for _ in range(90):
        x, y = rng.randrange(30, 240), rng.randrange(40, 190)
        if is_land(x, y) and distance_to_coast(x, y, 5) > 5 and land_value(x, y) < 0.7:
            draw_tree(m, x, y)


def draw_mountain(m, cx, base, half, height):
    for y in range(base - height, base + 1):
        span = round(half * (y - (base - height)) / height)
        for x in range(cx - span, cx + span + 1):
            if y < base - height + height // 3:
                color = SNOW
            else:
                color = ROCK if x <= cx else ROCK_SHADE
            m.set(x, y, color)
        m.set(cx - span - 1, y, OUTLINE)
        m.set(cx + span + 1, y, OUTLINE)


def draw_mountains(m):
    for cx, base, half, height in ((96, 58, 9, 14), (160, 56, 10, 16), (110, 72, 7, 10), (146, 74, 8, 11), (84, 76, 6, 8), (172, 72, 6, 9)):
        draw_mountain(m, cx, base, half, height)


def draw_houses(m, cx, cy, count, seed):
    rng = random.Random(seed)
    for _ in range(count):
        x = cx + rng.randrange(-22, 23)
        y = cy + rng.randrange(-16, 17)
        if abs(x - cx) < 14 and abs(y - cy) < 14:
            continue
        if not is_land(x, y) or distance_to_coast(x, y, 3) <= 3:
            continue
        m.rect(x, y, x + 3, y + 2, WALL)
        m.rect(x, y - 1, x + 3, y - 1, ROOF)
        m.set(x + 1, y + 1, (60, 50, 70))


def draw_compass(m, cx, cy):
    for d in range(-6, 7):
        m.set(cx + d, cy, (200, 214, 240))
        m.set(cx, cy + d, (200, 214, 240))
    for d in range(1, 4):
        m.set(cx - d, cy - 6 + d, (255, 222, 84))
        m.set(cx + d, cy - 6 + d, (255, 222, 84))
    m.rect(cx - 1, cy - 1, cx + 1, cy + 1, (255, 222, 84))


def build_map():
    m = Layer()
    draw_ocean(m)
    draw_land(m)
    draw_forest(m)
    draw_mountains(m)
    # Casas perto de Rock in Rede (costa leste) e do Lollaparafuso (cidade do oeste)
    draw_houses(m, 204, 110, 26, 1)
    draw_houses(m, 84, 124, 30, 2)
    draw_compass(m, 26, 176)
    return m


# ---------------------------------------------------------------- icones


def icon_planeta(c):
    blue, wave = (40, 100, 200), (100, 214, 244)
    for left in (6, 22):
        c.rect(left, 19, left + 3, 28, blue)
        c.rect(left, 23, left + 3, 23, wave)
    c.rect(4, 17, 27, 20, (26, 50, 120))
    c.rect(4, 17, 27, 17, wave)
    for y in range(2, 18):
        for x in range(6, 27):
            dy = y - 10 + (x - 16) * 0.2
            ring = ((x - 16) / 10) ** 2 + (dy / 2.6) ** 2
            if abs(ring - 1) < 0.3 and dy <= 0:
                c.set(x, y, (232, 240, 255))
    for y in range(2, 18):
        for x in range(8, 25):
            d = math.hypot(x - 16, y - 10)
            if d <= 6.5:
                light = (x - 16) + (y - 10)
                c.set(x, y, (255, 196, 96) if light < -4 else (190, 80, 52) if light > 4 else (242, 132, 52))
    for y in range(2, 18):
        for x in range(6, 27):
            dy = y - 10 + (x - 16) * 0.2
            ring = ((x - 16) / 10) ** 2 + (dy / 2.6) ** 2
            if abs(ring - 1) < 0.3 and dy > 0:
                c.set(x, y, (232, 240, 255))


def icon_rede(c):
    red, red_dark, white = (200, 36, 52), (132, 20, 42), (245, 236, 232)
    for y in range(3, 29):
        for x in range(3, 29):
            d = math.hypot(x - 15.5, y - 16)
            if y <= 16 and 8 < d <= 13:
                c.set(x, y, red_dark if d > 12 else red)
    c.rect(3, 16, 7, 28, red)
    c.rect(24, 16, 28, 28, red)
    c.rect(3, 16, 28, 19, white)
    c.disc(16, 10, 4, (44, 96, 208))
    c.set(15, 9, (255, 255, 255))
    c.set(17, 11, (110, 170, 255))


def icon_lolla(c):
    stripes = [(250, 80, 160), (255, 210, 60), (60, 210, 230), (120, 230, 110)]
    for left in (6, 22):
        for y in range(14, 29):
            for x in range(left, left + 4):
                c.set(x, y, stripes[((x + y) // 2) % 4])
    c.rect(3, 4, 28, 14, (240, 70, 150))
    c.rect(3, 4, 28, 4, (255, 220, 70))
    c.rect(3, 14, 28, 14, (255, 220, 70))
    for x in range(6, 26, 4):
        c.rect(x, 8, x + 1, 10, (255, 255, 255))


def icon_morro(c):
    wood, wood_light = (130, 82, 44), (172, 114, 62)
    for left in (5, 23):
        c.rect(left, 18, left + 3, 28, wood)
        c.rect(left, 18, left, 28, wood_light)
    for y in range(3, 19):
        span = round(13 * (y - 3) / 15)
        for x in range(16 - span, 16 + span + 1):
            color = SNOW if y < 8 else (126, 124, 148) if y < 12 else (62, 140, 72)
            c.set(x, y, color)
    c.rect(3, 18, 28, 20, (110, 70, 38))
    c.rect(3, 18, 28, 18, wood_light)


ICONS = [icon_planeta, icon_rede, icon_lolla, icon_morro]


class IconCanvas(Layer):
    """Layer pequeno do tamanho de um icone."""

    def __init__(self):
        self.px = [[None] * ICON for _ in range(ICON)]

    def set(self, x, y, color):
        if 0 <= x < ICON and 0 <= y < ICON:
            self.px[y][x] = color

    def get(self, x, y):
        return self.px[y][x] if 0 <= x < ICON and 0 <= y < ICON else None

    def outline(self, color=OUTLINE):
        filled = {(x, y) for y in range(ICON) for x in range(ICON) if self.px[y][x] is not None}
        for x, y in filled:
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if (nx, ny) not in filled:
                    self.set(nx, ny, color)


def build_icons():
    icons = []
    for draw in ICONS:
        c = IconCanvas()
        draw(c)
        c.outline()
        icons.append(c)
    return icons


def write_icons(path, icons):
    rows = [[(0, 0, 0, 0)] * (ICON * len(icons)) for _ in range(ICON)]
    for index, icon in enumerate(icons):
        for y in range(ICON):
            for x in range(ICON):
                color = icon.px[y][x]
                if color:
                    rows[y][index * ICON + x] = color + (255,)
    write_png(path, ICON * len(icons), ICON, rows)


# ---------------------------------------------------------------- fases do Planeta Catraca

DUSK = [(58, 40, 110), (96, 52, 128), (150, 66, 132), (204, 92, 120), (240, 134, 104), (252, 180, 110)]
HORIZON = 96
SAND_TOP = 128


def draw_palm(b, x, base, lean):
    for step in range(30):
        tx = x + round(lean * step * step / 120)
        b.rect(tx, base - step, tx + 2, base - step, (120, 80, 46) if step % 4 else (90, 58, 34))
    top_x = x + round(lean * 29 * 29 / 120) + 1
    top_y = base - 30
    for angle in (-2.6, -2.0, -1.2, -0.5, 0.2):
        for r in range(1, 13):
            px = top_x + round(math.cos(angle) * r)
            py = top_y + round(math.sin(angle) * r * 0.6 + (r * r) / 30)
            b.rect(px, py, px + 1, py, (46, 128, 66) if r < 10 else (30, 96, 52))
    b.rect(top_x - 1, top_y, top_x + 1, top_y + 2, (110, 70, 40))


def build_phase_background():
    b = Layer()
    band = HORIZON / len(DUSK)
    for y in range(HORIZON):
        position = y / band
        index = min(int(position), len(DUSK) - 1)
        frac = position - int(position)
        for x in range(W):
            color = DUSK[index]
            if index + 1 < len(DUSK) and frac > 0.8 and (x + y) % 2 == 0:
                color = DUSK[index + 1]
            b.set(x, y, color)
    # Sol se pondo no mar
    for y in range(HORIZON - 14, HORIZON):
        for x in range(96, 160):
            if math.hypot(x - 128, y - HORIZON) <= 16:
                b.set(x, y, (255, 232, 150) if (y // 3) % 2 == 0 or y < HORIZON - 8 else (255, 196, 110))
    for y in range(HORIZON, SAND_TOP):
        for x in range(W):
            color = (54, 92, 168) if y < HORIZON + 10 else (44, 112, 176)
            if (x * 7 + y * 13) % 29 == 0:
                color = WAVE
            # Reflexo do sol
            if abs(x - 128) < 18 - (y - HORIZON) // 2 and y % 3 == 0:
                color = (255, 214, 140)
            b.set(x, y, color)
    for y in range(SAND_TOP, H):
        for x in range(W):
            if y < SAND_TOP + 3:
                color = (240, 244, 255) if (x + y * 3) % 7 < 4 else SHALLOW
            else:
                color = SAND if (x * 5 + y * 11) % 13 else SAND_SHADE
            b.set(x, y, color)
    palms = Layer()
    draw_palm(palms, 12, 150, 1)
    draw_palm(palms, 34, 140, -1)
    draw_palm(palms, 182, 136, 1)
    palms.outline()
    b.stamp(palms)
    # O portal do festival no fim da praia: a fase final fica na frente dele
    b.stamp(portal_planeta_catraca(196))
    return b


def write_layer(path, layer):
    write_png(path, W, H, layer.rgba_rows())


def write_preview(path, layer, scale=3):
    rows = []
    for row in layer.px:
        scaled = [(c or (0, 0, 0)) + (255,) for c in row for _ in range(scale)]
        rows.extend([scaled] * scale)
    write_png(path, W * scale, H * scale, rows)


if __name__ == "__main__":
    out_dir = sys.argv[1]
    world_map = build_map()
    icons = build_icons()
    phases = build_phase_background()
    write_layer(os.path.join(out_dir, "world_map.png"), world_map)
    write_icons(os.path.join(out_dir, "world_icons.png"), icons)
    write_layer(os.path.join(out_dir, "phases_w1.png"), phases)
    if len(sys.argv) > 2:
        preview_dir = sys.argv[2]
        write_preview(os.path.join(preview_dir, "world_map_preview.png"), world_map)
        write_preview(os.path.join(preview_dir, "phases_w1_preview.png"), phases)
        sheet = Layer()
        for index, icon in enumerate(icons):
            for y in range(ICON):
                for x in range(ICON):
                    sheet.set(8 + index * 40 + x, 8 + y, icon.px[y][x] or (24, 28, 48))
        write_preview(os.path.join(preview_dir, "world_icons_preview.png"), sheet, scale=4)
