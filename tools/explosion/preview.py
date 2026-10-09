"""Превью анимации: логотип -> пиксельный взрыв Minecraft поверх логотипа -> экран телеметрии.

Запуск: .venv/bin/python preview.py  (результат: out/preview.gif)
Кадры спрайта: первая строка src_old/particles.png (Minecraft 1.12.2), 8 кадров 8x8, (не коммитятся, см. .gitignore).
"""
import random
import re
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

W, H = 128, 64
ROOT = Path(__file__).parent
SRC = ROOT / "src"
OUT = ROOT / "out"

LOGO_HOLD = 25          # шагов (по 40 мс) логотип показывается до взрыва
STEP_MS = 40
SEED = 7


def load_logo() -> np.ndarray:
    """logo.h -> bool[H, W]. Байт = 8 пикселей по вертикали, bit0 сверху (формат GyverOLED)."""
    text = (ROOT / "../../IMBA_TV/logo.h").read_text()
    body = text[text.index("{", text.index("logo_bitmap")):]
    data = [int(x, 16) for x in re.findall(r"0x[0-9a-fA-F]{2}", body)]
    assert len(data) == W * H // 8, len(data)
    img = np.zeros((H, W), bool)
    for page in range(H // 8):
        for x in range(W):
            b = data[page * W + x]
            for bit in range(8):
                img[page * 8 + bit, x] = bool(b >> bit & 1)
    return img


def mock_telemetry(connected=True, dev_charge=100, charging=False, speed=0, pwm=0, euc_charge=100) -> np.ndarray:
    """Настоящий экран телеметрии: собирает display.cpp из прошивки на компьютере (host/harness)
    и берёт буфер OLED. Раскладка блоков в точности как на устройстве."""
    import subprocess
    host = ROOT / "host"
    if not (host / "harness").exists():
        subprocess.run(
            ["clang++", "-std=c++17", "-I.", f"-I{Path.home()}/Documents/Arduino/libraries/GyverOLED/src",
             "-I../../../IMBA_TV", "harness.cpp", "-o", "harness"], cwd=host, check=True)
    out = OUT / "telemetry.bin"
    OUT.mkdir(exist_ok=True)
    subprocess.run([str(host / "harness"), str(out), str(dev_charge), str(int(charging)), str(int(connected)),
                    str(speed), str(pwm), str(euc_charge)], check=True)
    buf = out.read_bytes()
    img = np.zeros((H, W), bool)
    for x in range(W):
        for y in range(H):
            img[y, x] = bool(buf[(y >> 3) + x * 8] >> (y & 7) & 1)  # раскладка GyverOLED: y/8 + x*8
    return img


def load_sprites():
    """Первая строка particles.png: 8 кадров 8x8 пиксельного всплеска."""
    atlas = np.array(Image.open(ROOT / "src_old" / "particles.png").convert("RGBA"))
    return [atlas[0:8, i * 8:(i + 1) * 8, 3] > 0 for i in range(8)]


def blit(dst: np.ndarray, mask: np.ndarray, cx: int, cy: int, size: int, value: bool):
    """Масштабирует маску 8x8 до size и рисует её в dst с центром (cx, cy)."""
    m = np.array(Image.fromarray(mask.astype(np.uint8) * 255).resize((size, size), Image.NEAREST)) > 0
    x0, y0 = cx - size // 2, cy - size // 2
    for yy in range(size):
        for xx in range(size):
            if m[yy, xx]:
                X, Y = x0 + xx, y0 + yy
                if 0 <= X < W and 0 <= Y < H:
                    dst[Y, X] = value


def build_blasts(rng):
    """Несколько крупных всплесков: 6 по сетке 3x2 (покрывают экран) + пара случайных."""
    blasts = []
    for cx in (21, 64, 107):
        for cy in (16, 48):
            blasts.append(dict(
                cx=cx + rng.randint(-6, 6), cy=cy + rng.randint(-5, 5),
                size=rng.choice([64, 72, 80]), start=rng.randint(0, 12), step=rng.choice([3, 3, 4]),
            ))
    for _ in range(2):
        blasts.append(dict(
            cx=rng.randint(20, 108), cy=rng.randint(14, 50),
            size=rng.choice([48, 56]), start=rng.randint(4, 16), step=3,
        ))
    return blasts


def render():
    rng = random.Random(SEED)
    logo, telem = load_logo(), mock_telemetry()
    sprites = load_sprites()
    blasts = build_blasts(rng)

    total_steps = max(b["start"] + 8 * b["step"] for b in blasts) + 2
    frames = [logo.copy() for _ in range(LOGO_HOLD)]
    revealed = np.zeros((H, W), bool)   # где уже проявилась телеметрия

    for t in range(total_steps):
        sprite_layer = np.zeros((H, W), bool)
        for b in blasts:
            k = (t - b["start"]) // b["step"]
            if 0 <= k < 8:
                blit(sprite_layer, sprites[k], b["cx"], b["cy"], b["size"], True)
                # на пике (пока шар цельный) под ним логотип сменяется телеметрией
                if k == 5:
                    yy, xx = np.ogrid[:H, :W]
                    revealed |= (xx - b["cx"]) ** 2 + (yy - b["cy"]) ** 2 <= (b["size"] * 0.55) ** 2
        base = np.where(revealed, telem, logo)
        frames.append(base | sprite_layer)

    frames.append(telem.copy())
    return frames


def save_gif(frames, path: Path, scale=6):
    OUT.mkdir(exist_ok=True)
    imgs = []
    for f in frames:
        rgb = np.zeros((H, W, 3), np.uint8)
        rgb[f] = (200, 230, 255)    # «холодный» белый OLED
        imgs.append(Image.fromarray(rgb).resize((W * scale, H * scale), Image.NEAREST))
    imgs[0].save(path, save_all=True, append_images=imgs[1:], duration=STEP_MS, loop=0)
    imgs[-1].save(OUT / "last.png")
    imgs[LOGO_HOLD + 10].save(OUT / "mid.png"); imgs[LOGO_HOLD + 22].save(OUT / "mid2.png")


if __name__ == "__main__":
    fr = render()
    print("кадров:", len(fr), "длительность, с:", len(fr) * STEP_MS / 1000)
    save_gif(fr, OUT / "preview.gif")
