"""Gera assets/sprites/player.png: o protagonista em quadros de 32x32.

Linha 0: poses normais. Linha 1: as mesmas poses atirando.
A ordem das colunas precisa bater com PlayerFrame em src/GameRender.cpp.

Uso: python tools/gen_player_sprite.py assets/sprites/player.png
"""
import struct
import sys
import zlib

SIZE = 32

SKIN = (242, 194, 155)
SKIN_SHADE = (214, 158, 120)
SKIN_SHINE = (255, 226, 200)
HAIR = (92, 58, 34)
HAIR_LIGHT = (124, 82, 51)
SHIRT = (66, 72, 90)
SHIRT_SHADE = (46, 51, 66)
LOGO = (92, 224, 138)
JEANS = (63, 111, 181)
JEANS_SHADE = (47, 84, 144)
SHOE = (232, 232, 232)
SOLE = (138, 138, 138)
FRAME = (20, 20, 28)
LENS = (191, 230, 255)
MOUTH = (168, 96, 76)
SCANNER = (154, 163, 178)
SCANNER_LIGHT = (255, 90, 90)
OUTLINE = (12, 12, 18)

# dx/dy deslocam o tronco; feet e hand sao as pontas de pernas e bracos.
POSES = [
    {"name": "idle0", "feet": ((12, 29), (18, 29)), "hand": (13, 21), "back_hand": (20, 21)},
    {"name": "idle1", "dy": 1, "feet": ((12, 29), (18, 29)), "hand": (13, 22), "back_hand": (20, 22)},
    {"name": "run0", "dx": 1, "feet": ((7, 27), (21, 29)), "hand": (18, 20), "back_hand": (9, 20), "hair": "back"},
    {"name": "run1", "dy": 1, "dx": 1, "feet": ((12, 29), (17, 27)), "hand": (14, 21), "back_hand": (15, 20), "hair": "back"},
    {"name": "run2", "dx": 1, "feet": ((21, 28), (8, 28)), "hand": (9, 20), "back_hand": (19, 20), "hair": "back"},
    {"name": "run3", "dy": 1, "dx": 1, "feet": ((17, 29), (12, 27)), "hand": (15, 20), "back_hand": (14, 21), "hair": "back"},
    {"name": "jump", "dy": -1, "feet": ((10, 26), (20, 24)), "hand": (9, 14), "back_hand": (23, 14), "hair": "down"},
    {"name": "fall", "feet": ((11, 28), (20, 27)), "hand": (8, 11), "back_hand": (24, 11), "hair": "up"},
    {"name": "dash", "dx": 3, "dy": 2, "feet": ((4, 29), (21, 29)), "hand": (8, 21), "back_hand": (21, 20), "hair": "flat"},
    {"name": "wall", "feet": ((6, 26), (9, 29)), "hand": (5, 15), "back_hand": (14, 21), "hair": "up"},
    {"name": "climb0", "feet": ((12, 29), (18, 25)), "hand": (11, 7), "back_hand": (21, 14), "hair": "down"},
    {"name": "climb1", "feet": ((12, 25), (18, 29)), "hand": (11, 14), "back_hand": (21, 7), "hair": "down"},
    {"name": "hurt", "dx": -2, "feet": ((9, 29), (22, 26)), "hand": (5, 13), "back_hand": (25, 12), "hair": "up", "mouth": True},
]


class Canvas:
    def __init__(self):
        self.pixels = {}

    def set(self, x, y, color):
        if 0 <= x < SIZE and 0 <= y < SIZE:
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


def draw_hair(c, dx, dy, mode):
    if mode == "back":
        c.limb(11 + dx, 7 + dy, 5 + dx, 12 + dy, 4, HAIR)
        c.limb(11 + dx, 8 + dy, 6 + dx, 12 + dy, 1, HAIR_LIGHT)
    elif mode == "flat":
        c.limb(11 + dx, 7 + dy, 2 + dx, 8 + dy, 4, HAIR)
        c.limb(11 + dx, 8 + dy, 3 + dx, 9 + dy, 1, HAIR_LIGHT)
    elif mode == "up":
        c.limb(11 + dx, 8 + dy, 7 + dx, 3 + dy, 4, HAIR)
        c.limb(11 + dx, 9 + dy, 8 + dx, 4 + dy, 1, HAIR_LIGHT)
    else:
        c.rect(9 + dx, 6 + dy, 12 + dx, 16 + dy, HAIR)
        c.rect(10 + dx, 17 + dy, 12 + dx, 18 + dy, HAIR)
        c.rect(10 + dx, 7 + dy, 10 + dx, 16 + dy, HAIR_LIGHT)


def draw_head(c, dx, dy, mouth_open):
    c.rect(12 + dx, 4 + dy, 20 + dx, 12 + dy, SKIN)
    c.clear(12 + dx, 4 + dy)
    c.clear(20 + dx, 4 + dy)
    c.clear(20 + dx, 12 + dy)
    # Topo calvo com brilho; o cabelo so comeca nos lados e atras
    c.rect(14 + dx, 4 + dy, 16 + dx, 4 + dy, SKIN_SHINE)
    c.rect(12 + dx, 6 + dy, 14 + dx, 7 + dy, HAIR)
    c.rect(12 + dx, 8 + dy, 13 + dx, 11 + dy, HAIR)
    c.set(13 + dx, 5 + dy, HAIR)
    c.set(14 + dx, 9 + dy, SKIN_SHADE)
    # Oculos
    c.rect(15 + dx, 7 + dy, 20 + dx, 9 + dy, FRAME)
    c.set(14 + dx, 8 + dy, FRAME)
    c.rect(16 + dx, 8 + dy, 17 + dx, 8 + dy, LENS)
    c.rect(19 + dx, 8 + dy, 20 + dx, 8 + dy, LENS)
    c.set(21 + dx, 10 + dy, SKIN_SHADE)
    if mouth_open:
        c.rect(18 + dx, 11 + dy, 19 + dx, 12 + dy, MOUTH)
    else:
        c.rect(18 + dx, 11 + dy, 19 + dx, 11 + dy, MOUTH)
    # Papada
    c.rect(14 + dx, 13 + dy, 19 + dx, 13 + dy, SKIN_SHADE)


def draw_torso(c, dx, dy):
    c.rect(11 + dx, 14 + dy, 20 + dx, 24 + dy, SHIRT)
    c.rect(21 + dx, 16 + dy, 22 + dx, 23 + dy, SHIRT)
    c.rect(23 + dx, 18 + dy, 23 + dx, 22 + dy, SHIRT)
    c.rect(11 + dx, 23 + dy, 22 + dx, 24 + dy, SHIRT_SHADE)
    c.rect(11 + dx, 14 + dy, 11 + dx, 22 + dy, SHIRT_SHADE)
    # Estampa de camiseta: "</>"
    c.set(15 + dx, 17 + dy, LOGO)
    c.set(14 + dx, 18 + dy, LOGO)
    c.set(15 + dx, 19 + dy, LOGO)
    c.set(19 + dx, 17 + dy, LOGO)
    c.set(20 + dx, 18 + dy, LOGO)
    c.set(19 + dx, 19 + dy, LOGO)
    c.set(18 + dx, 17 + dy, LOGO)
    c.set(17 + dx, 18 + dy, LOGO)
    c.set(16 + dx, 19 + dy, LOGO)


def draw_leg(c, hip_x, hip_y, foot, color):
    fx, fy = foot
    c.limb(hip_x, hip_y, fx, fy - 1, 4, color)
    c.rect(fx, fy, fx + 4, fy + 1, SHOE)
    c.rect(fx, fy + 2, fx + 4, fy + 2, SOLE) if fy + 2 < SIZE else None


def draw_arm(c, shoulder, hand):
    sx, sy = shoulder
    hx, hy = hand
    mx, my = (sx + hx) // 2, (sy + hy) // 2
    c.limb(sx, sy, mx, my, 3, SHIRT_SHADE)
    c.limb(mx, my, hx, hy, 3, SKIN)


def draw_scanner_arm(c, dx, dy):
    c.limb(16 + dx, 15 + dy, 22 + dx, 15 + dy, 3, SHIRT_SHADE)
    c.limb(22 + dx, 15 + dy, 25 + dx, 15 + dy, 3, SKIN)
    c.rect(26 + dx, 13 + dy, 29 + dx, 18 + dy, SCANNER)
    c.rect(29 + dx, 14 + dy, 29 + dx, 16 + dy, SCANNER_LIGHT)


def draw_frame(pose, shooting):
    c = Canvas()
    dx = pose.get("dx", 0)
    dy = pose.get("dy", 0)
    feet = pose["feet"]

    draw_hair(c, dx, dy, pose.get("hair", "down"))
    draw_arm(c, (18 + dx, 15 + dy), pose["back_hand"])
    draw_leg(c, 16 + dx, 24 + dy, feet[1], JEANS_SHADE)
    draw_leg(c, 12 + dx, 24 + dy, feet[0], JEANS)
    draw_torso(c, dx, dy)
    draw_head(c, dx, dy, pose.get("mouth", False))
    if shooting:
        draw_scanner_arm(c, dx, dy)
    else:
        draw_arm(c, (12 + dx, 15 + dy), pose["hand"])
    c.outline()
    return c


def write_png(path, width, height, rows):
    def chunk(tag, data):
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))

    raw = b"".join(b"\0" + bytes(value for pixel in row for value in pixel) for row in rows)
    with open(path, "wb") as output:
        output.write(b"\x89PNG\r\n\x1a\n")
        output.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)))
        output.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        output.write(chunk(b"IEND", b""))


def build_sheet(path, scale=1, background=None):
    columns = len(POSES)
    width = columns * SIZE * scale
    height = 2 * SIZE * scale
    empty = background + (255,) if background else (0, 0, 0, 0)
    rows = [[empty] * width for _ in range(height)]
    for row, shooting in enumerate((False, True)):
        for column, pose in enumerate(POSES):
            frame = draw_frame(pose, shooting)
            for (x, y), color in frame.pixels.items():
                for sy in range(scale):
                    for sx in range(scale):
                        px = (column * SIZE + x) * scale + sx
                        py = (row * SIZE + y) * scale + sy
                        rows[py][px] = color + (255,)
    write_png(path, width, height, rows)


if __name__ == "__main__":
    build_sheet(sys.argv[1])
    if len(sys.argv) > 2:
        build_sheet(sys.argv[2], scale=4, background=(24, 28, 48))
