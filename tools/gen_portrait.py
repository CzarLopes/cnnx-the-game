"""Converte a foto de um personagem em retrato de pixel art para a caixa de dialogo.

O retrato tem 64x64 px e tres expressoes lado a lado: sorrindo, falando e rindo.
A base sai da foto (recorte, reducao, paleta limitada e contorno). Olhos e boca
recebem retoques fixos, porque na reducao eles viram borrao.

Cada personagem e uma entrada em CHARACTERS, com coordenadas na foto original:
- crop: quadrado recortado em volta da cabeca e dos ombros;
- outline: poligono da cabeca, do pescoco e dos ombros, para tirar o fundo;
- retouch: funcao que redesenha olhos e boca em coordenadas do retrato.

Requer Pillow. Uso: python tools/gen_portrait.py celo assets/sprites [pasta_da_previa]
"""
import os
import sys

from PIL import Image, ImageDraw, ImageEnhance

SIZE = 64
COLORS = 22
OUTLINE = (14, 12, 18, 255)


def full_mask(config):
    left, top, right, bottom = config["crop"]
    mask = Image.new("L", (right - left, bottom - top), 0)
    draw = ImageDraw.Draw(mask)
    draw.polygon([(x - left, y - top) for x, y in config["outline"]], fill=255)
    return mask


def build_mask(config):
    mask = full_mask(config).resize((SIZE, SIZE), Image.BOX)
    return mask.point(lambda value: 255 if value >= 128 else 0)


def reduce_photo(path, config):
    photo = Image.open(path).convert("RGB").crop(config["crop"])
    photo = ImageEnhance.Contrast(photo).enhance(1.12)
    photo = ImageEnhance.Color(photo).enhance(0.92)
    # O fundo vira uma cor so, para a paleta limitada ficar toda no personagem
    flat = Image.new("RGB", photo.size, (24, 28, 48))
    photo = Image.composite(photo, flat, full_mask(config))
    small = photo.resize((SIZE, SIZE), Image.LANCZOS)
    quantized = small.quantize(colors=COLORS, method=Image.Quantize.MEDIANCUT, kmeans=4, dither=Image.Dither.NONE)
    return quantized.convert("RGBA")


def clean_noise(image, mask):
    # Pixel que nao combina com nenhum vizinho vira a cor mais comum em volta
    pixels = image.load()
    source = image.copy().load()
    for y in range(1, SIZE - 1):
        for x in range(1, SIZE - 1):
            if not mask.getpixel((x, y)):
                continue
            around = [source[x + dx, y + dy] for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))]
            if source[x, y] not in around:
                pixels[x, y] = max(set(around), key=around.count)


def apply_mask_and_outline(image, mask):
    pixels = image.load()
    for y in range(SIZE):
        for x in range(SIZE):
            if not mask.getpixel((x, y)):
                pixels[x, y] = (0, 0, 0, 0)
    edge = []
    for y in range(SIZE):
        for x in range(SIZE):
            if pixels[x, y][3] == 0:
                continue
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if 0 <= nx < SIZE and 0 <= ny < SIZE and pixels[nx, ny][3] == 0:
                    edge.append((x, y))
                    break
    for x, y in edge:
        # O contorno fica para dentro, sem cortar os ombros na borda de baixo
        if y < SIZE - 1:
            pixels[x, y] = OUTLINE


def paint(pixels, color, points):
    for x, y in points:
        pixels[x, y] = color


def row(y, x0, x1):
    return [(x, y) for x in range(x0, x1 + 1)]


def recolor_by_light(pixels, region, ramp, boost=1.3):
    # Troca a cor da foto por um tom da rampa com a mesma luminosidade
    for x, y in region:
        r, g, b, a = pixels[x, y]
        light = (r * 3 + g * 6 + b) / 2550
        pixels[x, y] = ramp[min(int(light * len(ramp) * boost), len(ramp) - 1)]


def is_grayish(color):
    r, g, b = color[:3]
    return max(r, g, b) - min(r, g, b) < 60 and max(r, g, b) > 70


def retouch_celo(pixels, expression):
    hair = [(30, 22, 20, 255), (52, 38, 32, 255), (78, 60, 50, 255), (112, 92, 82, 255)]
    skin = [(140, 74, 54, 255), (178, 104, 74, 255), (206, 134, 102, 255), (230, 170, 140, 255),
            (246, 204, 182, 255)]
    brow = (44, 28, 22, 255)
    eye = (36, 22, 18, 255)
    white = (240, 232, 222, 255)
    skin_shade = (178, 104, 74, 255)
    skin_deep = (140, 74, 54, 255)
    mouth = (104, 36, 34, 255)
    teeth = (242, 238, 230, 255)
    lip = (192, 102, 88, 255)

    # O topo do cabelo saiu acinzentado pela luz da foto; vira castanho escuro.
    # Linha do cabelo medida na foto: desce nas laterais e fica alta na testa.
    def hairline(x):
        if x <= 18:
            return 20
        if x <= 23:
            return 16
        if x <= 40:
            return 12 + (x - 24) // 12
        return 22
    recolor_by_light(pixels, [(x, y) for x in range(10, 50) for y in range(0, hairline(x) + 1)], hair, 1.0)
    # Sombras acinzentadas da foto no rosto e no pescoco voltam a ser pele
    grayish = [(x, y) for x in range(14, 44) for y in range(10, 54)
               if y > hairline(x) and pixels[x, y][3] and is_grayish(pixels[x, y])]
    recolor_by_light(pixels, grayish, skin, 1.1)

    # Sobrancelhas grossas, inclinadas como a cabeca na foto
    paint(pixels, brow, row(24, 19, 25) + row(25, 18, 19) + row(22, 30, 36) + [(37, 23)])
    if expression == "laugh":
        paint(pixels, eye, [(20, 28), (21, 27), (22, 27), (23, 27), (24, 28),
                            (32, 26), (33, 25), (34, 25), (35, 25), (36, 26)])
    else:
        paint(pixels, eye, row(27, 20, 25) + row(25, 31, 37))
        paint(pixels, white, [(21, 28), (24, 28), (32, 26), (35, 26)])
        paint(pixels, eye, row(28, 22, 23) + row(26, 33, 34) + [(22, 29), (33, 27)])
        paint(pixels, skin_shade, row(29, 20, 21) + row(27, 31, 32))

    # Nariz: sombra do lado escuro do rosto e narinas
    paint(pixels, skin_shade, [(28, 29), (28, 30), (27, 31), (27, 32)])
    paint(pixels, skin_deep, [(28, 34), (29, 34), (33, 34)])

    # Sorriso largo, mais alto do lado direito da tela, como a cabeca inclinada
    if expression == "talk":
        paint(pixels, mouth, [(25, 35), (25, 36), (37, 34), (37, 35)])
        paint(pixels, teeth, row(36, 26, 36) + row(35, 33, 36))
        paint(pixels, mouth, row(37, 26, 36) + row(38, 27, 35) + row(39, 28, 34))
        paint(pixels, lip, row(40, 28, 34) + row(35, 26, 32) + row(34, 33, 36))
    else:
        paint(pixels, mouth, [(25, 35), (25, 36), (37, 34), (37, 35)])
        paint(pixels, teeth, row(36, 26, 36) + row(35, 33, 36) + row(37, 27, 35))
        paint(pixels, lip, row(38, 27, 35) + row(35, 26, 32) + row(34, 33, 36))
    # Vincos do sorriso e queixo, que sumiam na pele clara da foto
    paint(pixels, skin_shade, [(23, 34), (24, 35), (38, 32), (39, 33)])
    paint(pixels, skin_deep, row(49, 26, 34) + [(24, 48), (25, 48), (35, 48), (36, 47)])


CHARACTERS = {
    "celo": {
        "photo": "assets/sprites/inspiration/Celo-logistica.png",
        "output": "portrait_celo.png",
        "crop": (165, 32, 370, 237),
        "outline": [(237, 52), (252, 45), (275, 45), (295, 53), (305, 64), (309, 77), (310, 95),
                    (312, 107), (320, 110), (321, 125), (316, 138), (312, 150), (322, 155), (345, 160),
                    (370, 172), (370, 237), (165, 237), (165, 224), (226, 176), (222, 160), (219, 140),
                    (215, 120), (214, 102), (214, 85), (219, 72), (227, 60)],
        "retouch": retouch_celo,
    },
}


def build_portrait(config, expression, mask, base):
    image = base.copy()
    config["retouch"](image.load(), expression)
    apply_mask_and_outline(image, mask)
    return image


def save_strip(images, path, scale=1, background=None):
    strip = Image.new("RGBA", (SIZE * len(images) * scale, SIZE * scale), background or (0, 0, 0, 0))
    for index, image in enumerate(images):
        frame = image.resize((SIZE * scale, SIZE * scale), Image.NEAREST)
        strip.alpha_composite(frame, (index * SIZE * scale, 0))
    strip.save(path)


if __name__ == "__main__":
    config = CHARACTERS[sys.argv[1]]
    mask = build_mask(config)
    base = reduce_photo(config["photo"], config)
    clean_noise(base, mask)
    portraits = [build_portrait(config, expression, mask, base) for expression in ("smile", "talk", "laugh")]
    save_strip(portraits, os.path.join(sys.argv[2], config["output"]))
    if len(sys.argv) > 3:
        save_strip(portraits, os.path.join(sys.argv[3], "previa_" + config["output"]), 6, (24, 28, 48, 255))
