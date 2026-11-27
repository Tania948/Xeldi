#!/usr/bin/env python3
"""Genera el arte de Xeldi (mascota, iconos de la app e iconos de la UI).

Todo el arte es original y se define aqui como texto/primitivas para que sea
facil de versionar y editar. Requiere Pillow:  python3 tools/gen_assets.py
"""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "assets"

PAL = {
    ".": (0, 0, 0, 0),
    "o": (0x5A, 0x3D, 0x52, 255),  # contorno ciruela
    "w": (0xFF, 0xF6, 0xF8, 255),  # pelaje
    "b": (0xFF, 0xFF, 0xFF, 255),  # pancita
    "k": (0xF2, 0x9B, 0xB8, 255),  # interior de oreja
    "c": (0xF7, 0xB5, 0xC8, 255),  # mejillas
    "e": (0x3B, 0x2A, 0x3A, 255),  # ojos
    "h": (0xFF, 0xFF, 0xFF, 255),  # brillo
    "n": (0xE8, 0x6A, 0x92, 255),  # nariz
    "l": (0xC9, 0xA8, 0xE8, 255),  # monito lavanda
    "L": (0xA7, 0x75, 0xC9, 255),  # nudo del monito
}

# Mitad izquierda del conejito (32x32). Se refleja para obtener la derecha.
BUNNY_LEFT = [
    "................",
    "..........ooo...",
    ".........owwwo..",
    ".........owkwo..",
    ".........owkwo..",
    ".........owkwo..",
    ".........owkwo..",
    ".........owkwo..",
    ".........owkwo..",
    "........owwkwo..",
    "......ooowwwwooo",
    ".....owwwwwwwwww",
    "....owwwwwwwwwww",
    "...owwwwwwwwwwww",
    "...owwwwhewwwwww",
    "...owwwweewwwwww",
    "...owwwweewwwwww",
    "...owccwwwwwwwwn",
    "....owwwwwwwwwow",
    ".....owwwwwwwwwo",
    "......oowwwwwwww",
    "........oooowwww",
    ".......owwwwbbbb",
    "......owwowwbbbb",
    "......owwowwbbbb",
    ".......oowwwbbbb",
    ".......owwwwbbbb",
    ".......owwwwwwww",
    "........owwwwwoo",
    ".......owwwwwwo.",
    ".......oooooooo.",
    "................",
]


def bunny_rows():
    rows = []
    for r in BUNNY_LEFT:
        assert len(r) == 16, r
        rows.append(list(r + r[::-1]))
    # Los brillos de los ojos miran al mismo lado.
    rows[14][22], rows[14][23] = "h", "e"
    # Monito lavanda en la oreja derecha.
    bow = {(4, 18): "l", (4, 19): "l", (4, 21): "l", (4, 22): "l",
           (5, 18): "l", (5, 19): "l", (5, 20): "L", (5, 21): "l", (5, 22): "l",
           (6, 18): "l", (6, 19): "l", (6, 21): "l", (6, 22): "l"}
    for (r, c), ch in bow.items():
        rows[r][c] = ch
    return rows


def to_image(rows):
    img = Image.new("RGBA", (len(rows[0]), len(rows)))
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            img.putpixel((x, y), PAL[ch])
    return img


def idle_frames():
    base = bunny_rows()
    breathe = [list("." * 32)] + [r[:] for r in base[:22]] + [r[:] for r in base[23:]]
    breathe = breathe[:32]
    blink = [r[:] for r in base]
    for c in (8, 9, 22, 23):
        blink[14][c] = "w"
        blink[15][c] = "e"
        blink[16][c] = "w"
    return [base, breathe, base, blink]


def scale(img, s):
    return img.resize((img.width * s, img.height * s), Image.NEAREST)


def app_icon():
    """Conejito sobre un cuadro rosa redondeado con contorno pixelado."""
    icon = Image.new("RGBA", (32, 32))
    d = ImageDraw.Draw(icon)
    d.rounded_rectangle((1, 3, 30, 31), radius=5, fill=(0xF7, 0xD6, 0xE0, 255), outline=PAL["o"])
    icon.alpha_composite(to_image(bunny_rows()), (0, -1))
    return icon


def small_icon():
    """Version 16x16 (solo cabecita) para tamanos chicos."""
    rows = [
        "...oo......oo...",
        "..owko....okwo..",
        "..owko....okwo..",
        "..owko....okwo..",
        "..owwoooooowwo..",
        ".owwwwwwwwwwwwo.",
        "owwwwwwwwwwwwwwo",
        "owwweewwwwwweewo",
        "owwweewwwwwweewo",
        "owccwwwnnwwwccwo",
        "owwwwwowwowwwwwo",
        ".owwwwwoowwwwwo.",
        "..oowwwwwwwwoo..",
        "....oooooooo....",
        "................",
        "................",
    ]
    rows = [r.replace(".", ".") for r in rows]
    fixed = []
    for r in rows:
        fixed.append(r.replace("h.", "hw"))
    return to_image([list(r) for r in fixed])


# ---------------------------------------------------------------- iconos UI
O = PAL["o"]
PINK = (0xF2, 0x9B, 0xB8, 255)
DPINK = (0xE8, 0x6A, 0x92, 255)
LPINK = (0xF7, 0xD6, 0xE0, 255)
BLUE = (0xB8, 0xC4, 0xFF, 255)
DBLUE = (0x8B, 0x9B, 0xE0, 255)
LAV = (0xC9, 0xA8, 0xE8, 255)
DLAV = (0xA7, 0x75, 0xC9, 255)
CAFE = (0xC8, 0xAD, 0x9C, 255)
CREAM = (0xFF, 0xEE, 0xDD, 255)
WHITE = (255, 255, 255, 255)
MINT = (0xA8, 0xE0, 0xC8, 255)


def canvas():
    img = Image.new("RGBA", (16, 16))
    return img, ImageDraw.Draw(img)


def px(d, pts, col):
    for p in pts:
        d.point(p, fill=col)


def ic_pencil():
    img, d = canvas()
    for i in range(9):
        x, y = 4 + i, 11 - i
        px(d, [(x, y), (x + 1, y), (x, y + 1), (x + 1, y + 1)], PINK)
        px(d, [(x - 1, y), (x + 2, y + 1)], O)
    px(d, [(12, 1), (13, 2), (13, 1), (14, 2), (14, 3)], O)
    px(d, [(12, 2), (13, 3)], LAV)
    px(d, [(3, 12), (2, 13), (3, 13), (2, 14)], O)
    px(d, [(4, 12), (3, 11)], CREAM)
    return img


def ic_eraser():
    img, d = canvas()
    d.polygon([(2, 10), (8, 4), (14, 10), (8, 15)], fill=LPINK, outline=O)
    d.polygon([(2, 10), (5, 7), (11, 13), (8, 15)], fill=PINK, outline=O)
    d.line([(1, 15), (14, 15)], fill=CAFE)
    return img


def ic_line():
    img, d = canvas()
    d.line([(2, 13), (13, 2)], fill=DPINK)
    d.rectangle((1, 12, 3, 14), outline=O)
    d.rectangle((12, 1, 14, 3), outline=O)
    return img


def ic_rect(filled=False):
    img, d = canvas()
    d.rectangle((2, 3, 13, 12), fill=PINK if filled else None, outline=O)
    if not filled:
        d.rectangle((3, 4, 12, 11), outline=PINK)
    return img


def ic_ellipse():
    img, d = canvas()
    d.ellipse((1, 3, 14, 12), outline=O)
    d.ellipse((2, 4, 13, 11), outline=PINK)
    return img


def ic_fill():
    img, d = canvas()
    d.polygon([(3, 7), (8, 2), (13, 7), (8, 12)], fill=CREAM, outline=O)
    d.line([(4, 8), (12, 8)], fill=BLUE)
    d.polygon([(4, 9), (8, 12), (12, 8), (12, 9), (8, 13)], fill=BLUE)
    px(d, [(13, 9), (13, 10), (14, 11), (13, 11), (14, 12), (13, 12)], DBLUE)
    return img


def ic_picker():
    img, d = canvas()
    for i in range(7):
        px(d, [(3 + i, 12 - i), (4 + i, 12 - i)], LAV)
        px(d, [(3 + i, 11 - i), (5 + i, 12 - i)], O)
    d.ellipse((9, 1, 14, 6), fill=DLAV, outline=O)
    px(d, [(2, 13), (1, 14)], O)
    px(d, [(2, 14)], DPINK)
    return img


def iso_cube(d, ox, oy, top, left, right):
    d.polygon([(ox + 6, oy), (ox + 12, oy + 3), (ox + 6, oy + 6), (ox, oy + 3)], fill=top, outline=O)
    d.polygon([(ox, oy + 3), (ox + 6, oy + 6), (ox + 6, oy + 13), (ox, oy + 10)], fill=left, outline=O)
    d.polygon([(ox + 6, oy + 6), (ox + 12, oy + 3), (ox + 12, oy + 10), (ox + 6, oy + 13)], fill=right, outline=O)


def ic_cube_add():
    img, d = canvas()
    iso_cube(d, 1, 2, LPINK, PINK, DPINK)
    d.rectangle((10, 0, 15, 5), fill=MINT, outline=O)
    px(d, [(12, 2), (13, 2), (12, 3), (13, 3)], WHITE)
    return img


def ic_cube_remove():
    img, d = canvas()
    iso_cube(d, 1, 2, LPINK, PINK, DPINK)
    d.rectangle((10, 0, 15, 5), fill=CAFE, outline=O)
    d.line([(11, 2), (14, 2)], fill=WHITE)
    d.line([(11, 3), (14, 3)], fill=WHITE)
    return img


def ic_cube_paint():
    img, d = canvas()
    iso_cube(d, 1, 2, (0xDD, 0xE3, 0xFF, 255), BLUE, DBLUE)
    d.line([(9, 1), (14, 6)], fill=O)
    d.rectangle((12, 0, 15, 3), fill=DPINK, outline=O)
    return img


def plane(d, fill):
    d.polygon([(1, 9), (7, 5), (15, 8), (9, 13)], fill=fill, outline=O)


def ic_plane_draw():
    img, d = canvas()
    plane(d, (0xE3, 0xD4, 0xF7, 255))
    d.line([(4, 9), (8, 10)], fill=DPINK)
    for i in range(5):
        px(d, [(9 + i, 6 - i), (10 + i, 6 - i)], PINK)
    px(d, [(8, 7)], O)
    return img


def ic_plane_erase():
    img, d = canvas()
    plane(d, (0xE3, 0xD4, 0xF7, 255))
    d.polygon([(8, 4), (11, 1), (15, 5), (12, 8)], fill=LPINK, outline=O)
    return img


def ic_extrude():
    img, d = canvas()
    plane(d, (0xE3, 0xD4, 0xF7, 255))
    d.rectangle((6, 3, 9, 9), fill=PINK, outline=O)
    d.line([(7, 0), (7, 2)], fill=DLAV)
    px(d, [(6, 1), (8, 1)], DLAV)
    return img


def ic_play():
    img, d = canvas()
    d.polygon([(4, 2), (13, 8), (4, 14)], fill=PINK, outline=O)
    px(d, [(5, 4), (5, 5), (6, 5)], WHITE)
    return img


def ic_pause():
    img, d = canvas()
    d.rectangle((3, 2, 6, 13), fill=BLUE, outline=O)
    d.rectangle((9, 2, 12, 13), fill=BLUE, outline=O)
    return img


def ic_onion():
    img, d = canvas()
    d.ellipse((0, 4, 9, 13), outline=PINK)
    d.ellipse((6, 4, 15, 13), outline=BLUE)
    d.ellipse((3, 3, 12, 14), fill=LPINK, outline=O)
    px(d, [(6, 6), (7, 6), (6, 7)], WHITE)
    return img


def ic_simple(kind):
    img, d = canvas()
    if kind == "layer_add":
        d.polygon([(1, 9), (8, 5), (15, 9), (8, 13)], fill=LAV, outline=O)
        d.polygon([(1, 6), (8, 2), (15, 6), (8, 10)], fill=LPINK, outline=O)
        d.rectangle((11, 11, 15, 15), fill=MINT, outline=O)
    elif kind == "frame_add":
        d.rectangle((1, 2, 11, 12), fill=CREAM, outline=O)
        d.rectangle((3, 4, 9, 10), fill=LPINK)
        d.rectangle((10, 10, 15, 15), fill=MINT, outline=O)
    elif kind == "frame_dup":
        d.rectangle((4, 1, 14, 11), fill=LPINK, outline=O)
        d.rectangle((1, 4, 11, 14), fill=CREAM, outline=O)
    elif kind == "trash":
        d.rectangle((3, 4, 12, 14), fill=LPINK, outline=O)
        d.line([(2, 3), (13, 3)], fill=O)
        d.rectangle((6, 1, 9, 2), outline=O)
        for x in (5, 8, 11):
            d.line([(x - 0, 6), (x - 0, 12)], fill=PINK)
    elif kind == "undo":
        d.arc((3, 3, 14, 14), 180, 90, fill=O)
        d.polygon([(1, 8), (5, 4), (5, 8)], fill=DPINK, outline=O)
    elif kind == "redo":
        d.arc((1, 3, 12, 14), 90, 0, fill=O)
        d.polygon([(14, 8), (10, 4), (10, 8)], fill=DPINK, outline=O)
    elif kind == "eye":
        d.ellipse((1, 4, 14, 11), fill=WHITE, outline=O)
        d.ellipse((5, 5, 10, 10), fill=DLAV)
        px(d, [(6, 6)], WHITE)
    elif kind == "eye_off":
        d.line([(1, 8), (14, 8)], fill=O)
        d.arc((1, 3, 14, 12), 20, 160, fill=O)
    elif kind == "lock":
        d.rectangle((3, 7, 12, 14), fill=CAFE, outline=O)
        d.arc((4, 2, 11, 11), 180, 0, fill=O)
    elif kind == "grid":
        d.rectangle((1, 1, 14, 14), fill=WHITE, outline=O)
        for v in (5, 10):
            d.line([(v, 2), (v, 13)], fill=LAV)
            d.line([(2, v), (13, v)], fill=LAV)
        d.rectangle((6, 6, 9, 9), fill=PINK)
    elif kind == "swap":
        d.line([(3, 4), (12, 4)], fill=O)
        d.polygon([(12, 1), (15, 4), (12, 7)], fill=PINK, outline=O)
        d.line([(3, 11), (12, 11)], fill=O)
        d.polygon([(3, 8), (0, 11), (3, 14)], fill=BLUE, outline=O)
    return img


UI_ICONS = {
    "pencil": ic_pencil, "eraser": ic_eraser, "line": ic_line, "rect": ic_rect,
    "rect_filled": lambda: ic_rect(True), "ellipse": ic_ellipse, "fill": ic_fill, "picker": ic_picker,
    "cube_add": ic_cube_add, "cube_remove": ic_cube_remove, "cube_paint": ic_cube_paint,
    "plane_draw": ic_plane_draw, "plane_erase": ic_plane_erase, "extrude": ic_extrude,
    "play": ic_play, "pause": ic_pause, "onion": ic_onion,
}
for k in ("layer_add", "frame_add", "frame_dup", "trash", "undo", "redo", "eye", "eye_off", "lock", "swap", "grid"):
    UI_ICONS[k] = (lambda kk: (lambda: ic_simple(kk)))(k)


def main():
    (ASSETS / "mascot").mkdir(parents=True, exist_ok=True)
    (ASSETS / "ui").mkdir(parents=True, exist_ok=True)
    frames = [to_image(f) for f in idle_frames()]
    for i, f in enumerate(frames):
        f.save(ASSETS / "mascot" / f"bunny_idle_{i}.png")
    sheet = Image.new("RGBA", (32 * len(frames), 32))
    for i, f in enumerate(frames):
        sheet.paste(f, (32 * i, 0))
    sheet.save(ASSETS / "mascot" / "bunny_idle_sheet.png")
    scale(frames[0], 8).save(ASSETS / "mascot" / "bunny_256.png")

    icon = app_icon()
    small = small_icon()
    for size in (16, 32, 48, 64, 128, 256, 512):
        out = ASSETS / "icons" / "hicolor" / f"{size}x{size}" / "apps"
        out.mkdir(parents=True, exist_ok=True)
        if size == 16:
            img = small
        elif size % 32 == 0:
            img = scale(icon, size // 32)
        else:  # 48: 3x de la version de 16
            img = scale(small, size // 16)
        img.save(out / "xeldi.png")

    for name, fn in UI_ICONS.items():
        fn().save(ASSETS / "ui" / f"{name}.png")
    # Hoja de previsualizacion para el README.
    names = list(UI_ICONS)
    prev = Image.new("RGBA", (len(names) * 20, 20), (0xF7, 0xD6, 0xE0, 255))
    for i, n in enumerate(names):
        prev.alpha_composite(UI_ICONS[n](), (i * 20 + 2, 2))
    scale(prev, 3).save(ROOT / "docs" / "img_iconos_ui.png")
    print("ok:", len(frames), "frames,", len(names), "iconos UI")


if __name__ == "__main__":
    main()
