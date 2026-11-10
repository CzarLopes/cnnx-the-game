"""Gera o corpo do Celo, da logistica: o personagem que passa as dicas.

assets/sprites/npc_celo.png: corpo inteiro em quadros de 32x32, de lado.
O retrato da caixa de dialogo sai da foto, por tools/gen_portrait.py.

Referencia: assets/sprites/inspiration/Celo-logistica.png. Cabelo curto
escuro com degrade nas laterais, sorriso aberto, camiseta preta com
etiqueta no peito, correntinha dourada, bermuda jeans clara rasgada e celular.

Uso: python tools/gen_npc_sprite.py assets/sprites [pasta_da_previa]
"""
import os
import sys

from gen_player_sprite import write_png

SKIN = (234, 186, 146)
SKIN_SHADE = (203, 148, 110)
HAIR = (38, 28, 24)
HAIR_LIGHT = (70, 54, 44)
FADE = (104, 86, 76)
BROW = (46, 32, 26)
EYE = (52, 34, 24)
MOUTH = (150, 68, 62)
TEETH = (252, 252, 248)
SHIRT = (40, 40, 48)
SHIRT_SHADE = (26, 26, 32)
SHIRT_LIGHT = (64, 64, 76)
PATCH = (120, 120, 128)
CHAIN = (236, 198, 92)
SHORTS = (146, 190, 228)
SHORTS_SHADE = (108, 152, 198)
RIP = (232, 240, 248)
SHOE = (236, 236, 236)
SOLE = (138, 138, 138)
PHONE = (48, 48, 58)
SCREEN = (130, 210, 255)
OUTLINE = (12, 12, 18)


class Canvas:
    def __init__(self, size):
        self.size = size
        self.pixels = {}

    def set(self, x, y, color):
        if 0 <= x < self.size and 0 <= y < self.size:
            self.pixels[(x, y)] = color

    def clear(self, x, y):
        self.pixels.pop((x, y), None)

    def rect(self, x0, y0, x1, y1, color):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.set(x, y, color)

    def limb(self, x0, y0, x1, y1, width, color):
        steps = max(abs(x1 - x0), abs(y1 - y0), 1)
        for step in range(steps + 1):
            x = round(x0 + (x1 - x0) * step / steps)
            y = round(y0 + (y1 - y0) * step / steps)
            self.rect(x, y, x + width - 1, y + width - 1, color)

    def outline(self):
        filled = set(self.pixels)
        for x, y in filled:
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if (nx, ny) not in filled:
                    self.set(nx, ny, OUTLINE)


# Corpo inteiro, de lado e olhando para a direita, como o protagonista.
# A ordem das colunas precisa bater com NpcFrame quando ele entrar no jogo.
POSES = [
    {"name": "idle0", "hand": (18, 22), "back_hand": (14, 22)},
    {"name": "idle1", "dy": 1, "hand": (18, 23), "back_hand": (14, 23)},
    {"name": "talk0", "hand": (18, 22), "back_hand": (14, 22), "mouth": "open"},
    {"name": "talk1", "hand": (23, 17), "back_hand": (14, 22), "mouth": "open"},
    {"name": "wave", "hand": (23, 7), "back_hand": (14, 22), "mouth": "smile"},
    {"name": "phone", "hand": (21, 18), "back_hand": (14, 22), "phone": True, "look": "down"},
]


def draw_body_head(c, dx, dy, mouth, look):
    c.rect(13 + dx, 4 + dy, 21 + dx, 13 + dy, SKIN)
    c.clear(21 + dx, 13 + dy)
    c.clear(13 + dx, 13 + dy)
    c.set(22 + dx, 9 + dy, SKIN)
    # Cabelo curto com volume em cima e degrade na lateral e na nuca
    c.rect(12 + dx, 3 + dy, 20 + dx, 4 + dy, HAIR)
    c.rect(14 + dx, 2 + dy, 20 + dx, 2 + dy, HAIR)
    c.rect(16 + dx, 1 + dy, 19 + dx, 1 + dy, HAIR)
    c.rect(15 + dx, 2 + dy, 18 + dx, 2 + dy, HAIR_LIGHT)
    c.set(21 + dx, 3 + dy, HAIR)
    c.rect(12 + dx, 5 + dy, 14 + dx, 6 + dy, HAIR)
    c.rect(12 + dx, 7 + dy, 13 + dx, 10 + dy, FADE)
    c.set(15 + dx, 5 + dy, HAIR)
    # Orelha
    c.rect(14 + dx, 8 + dy, 15 + dx, 10 + dy, SKIN_SHADE)
    c.set(15 + dx, 9 + dy, SKIN)
    eye_y = 9 if look == "down" else 8
    c.rect(18 + dx, 6 + dy, 20 + dx, 6 + dy, BROW)
    c.set(19 + dx, eye_y + dy, EYE)
    c.set(20 + dx, eye_y + dy, EYE)
    c.set(21 + dx, 10 + dy, SKIN_SHADE)
    if mouth == "open":
        c.rect(18 + dx, 11 + dy, 21 + dx, 12 + dy, MOUTH)
        c.rect(19 + dx, 11 + dy, 21 + dx, 11 + dy, TEETH)
    else:
        # Sorriso largo, mostrando os dentes como na foto
        c.set(18 + dx, 11 + dy, MOUTH)
        c.rect(19 + dx, 12 + dy, 21 + dx, 12 + dy, MOUTH)
        c.rect(19 + dx, 11 + dy, 21 + dx, 11 + dy, TEETH)
    c.rect(15 + dx, 12 + dy, 18 + dx, 13 + dy, SKIN_SHADE)


def draw_body_torso(c, dx, dy):
    c.rect(13 + dx, 14 + dy, 21 + dx, 21 + dy, SHIRT)
    c.rect(13 + dx, 14 + dy, 13 + dx, 21 + dy, SHIRT_SHADE)
    c.rect(20 + dx, 15 + dy, 20 + dx, 20 + dy, SHIRT_LIGHT)
    # Gola, correntinha e etiqueta no peito
    c.rect(16 + dx, 14 + dy, 19 + dx, 14 + dy, SKIN)
    c.rect(16 + dx, 15 + dy, 18 + dx, 15 + dy, CHAIN)
    c.set(17 + dx, 16 + dy, CHAIN)
    c.rect(18 + dx, 17 + dy, 19 + dx, 18 + dy, PATCH)


def draw_body_leg(c, hip_x, hip_y, foot, shade):
    fx, fy = foot
    knee_y = hip_y + 4
    c.rect(hip_x, hip_y, hip_x + 3, knee_y, SHORTS_SHADE if shade else SHORTS)
    if not shade:
        c.set(hip_x + 1, hip_y + 2, RIP)
        c.set(hip_x + 2, hip_y + 3, RIP)
    c.limb(hip_x + 1, knee_y + 1, fx + 1, fy - 1, 2, SKIN_SHADE if shade else SKIN)
    c.rect(fx, fy, fx + 3, fy + 1, SHOE)
    c.rect(fx, fy + 2, fx + 3, fy + 2, SOLE) if fy + 2 < c.size else None


def draw_body_arm(c, shoulder, hand, shade):
    sx, sy = shoulder
    hx, hy = hand
    mx = sx + (hx - sx) // 3
    my = sy + (hy - sy) // 3
    c.limb(sx, sy, mx, my, 3, SHIRT_SHADE if shade else SHIRT)
    c.limb(mx, my, hx, hy, 2, SKIN_SHADE if shade else SKIN)


def draw_body(pose):
    c = Canvas(32)
    dx = pose.get("dx", 0)
    dy = pose.get("dy", 0)
    draw_body_arm(c, (15 + dx, 15 + dy), pose["back_hand"], True)
    draw_body_leg(c, 14 + dx, 21 + dy, (14, 29), True)
    draw_body_leg(c, 17 + dx, 21 + dy, (17, 29), False)
    draw_body_torso(c, dx, dy)
    draw_body_head(c, dx, dy, pose.get("mouth", "smile"), pose.get("look"))
    draw_body_arm(c, (18 + dx, 15 + dy), pose["hand"], False)
    if pose.get("phone"):
        hx, hy = pose["hand"]
        c.rect(hx + 1, hy - 4, hx + 2, hy, PHONE)
        c.rect(hx + 1, hy - 3, hx + 1, hy - 1, SCREEN)
    if pose["name"] == "wave":
        hx, hy = pose["hand"]
        c.rect(hx, hy - 2, hx + 2, hy, SKIN)
        c.set(hx, hy - 3, SKIN)
        c.set(hx + 2, hy - 3, SKIN)
    c.outline()
    return c


def build_sheet(path, frames, size, scale=1, background=None):
    width = len(frames) * size * scale
    height = size * scale
    empty = background + (255,) if background else (0, 0, 0, 0)
    rows = [[empty] * width for _ in range(height)]
    for column, frame in enumerate(frames):
        for (x, y), color in frame.pixels.items():
            for sy in range(scale):
                for sx in range(scale):
                    rows[y * scale + sy][(column * size + x) * scale + sx] = color + (255,)
    write_png(path, width, height, rows)


if __name__ == "__main__":
    bodies = [draw_body(pose) for pose in POSES]
    build_sheet(os.path.join(sys.argv[1], "npc_celo.png"), bodies, 32)
    if len(sys.argv) > 2:
        build_sheet(os.path.join(sys.argv[2], "celo_corpo_previa.png"), bodies, 32, 6, (24, 28, 48))
