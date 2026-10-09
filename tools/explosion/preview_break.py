"""Превью перехода «разрушение блока» (Minecraft).

Логотип держится -> на нём проступают трещины (destroy_stage_0..9, два блока 64x64 рядом)
-> логотип ломается на кусочки 8x8, они разлетаются и падают -> под ними настоящий интерфейс.
Запуск: .venv/bin/python preview_break.py  (результат: out/break.gif)
Текстуры берутся из src_destroy/ (не коммитятся, см. .gitignore).
"""
import random

import numpy as np
from PIL import Image

from preview import H, OUT, ROOT, STEP_MS, W, load_logo, mock_telemetry
from preview_wave import SCALE

LOGO_HOLD = 25
STEPS_PER_STAGE = 2
BREAK_STEPS = 28
TILE = 8
GRAVITY = 0.32
SEED = 11


def load_cracks():
    """10 масок 16x16: трещина = непрозрачный пиксель текстуры."""
    out = []
    for i in range(10):
        a = np.array(Image.open(ROOT / "src_destroy" / f"destroy_stage_{i}.png").convert("RGBA"))
        out.append((a[..., 3] == 255) & (a[..., 0] < 100))   # только тёмные пиксели трещин
    return out


def crack_mask(cracks, stage) -> np.ndarray:
    """Сетка блоков 32x32 (4x2), трещина x2. Блоки отражены/повёрнуты для разнообразия."""
    base = np.kron(cracks[stage], np.ones((2, 2), bool))
    m = np.zeros((H, W), bool)
    for by in range(H // 32):
        for bx in range(W // 32):
            blk = np.rot90(base, (bx + 2 * by) % 4)
            m[by * 32:(by + 1) * 32, bx * 32:(bx + 1) * 32] = blk
    return m


def make_tiles(logo, rng):
    tiles = []
    for ty in range(H // TILE):
        for tx in range(W // TILE):
            t = logo[ty * TILE:(ty + 1) * TILE, tx * TILE:(tx + 1) * TILE]
            if not t.any():
                continue
            cx = tx * TILE + TILE / 2
            tiles.append(dict(
                pix=t, x=tx * TILE, y=ty * TILE,
                vx=(cx - W / 2) / (W / 2) * 2.0 + rng.uniform(-0.6, 0.6),
                vy=-rng.uniform(1.0, 3.2),
                life=rng.randint(12, BREAK_STEPS),
            ))
    return tiles


def blit_tile(frame, tile, px, py):
    for yy in range(TILE):
        for xx in range(TILE):
            X, Y = px + xx, py + yy
            if 0 <= X < W and 0 <= Y < H:
                frame[Y, X] = tile["pix"][yy, xx]     # кусочек сначала стирает фон под собой


def render(telem):
    rng = random.Random(SEED)
    logo = load_logo()
    cracks = load_cracks()
    tiles = make_tiles(logo, rng)

    frames = [logo.copy() for _ in range(LOGO_HOLD)]
    for stage in range(10):
        for _ in range(STEPS_PER_STAGE):
            frames.append(logo & ~crack_mask(cracks, stage))

    for t in range(BREAK_STEPS):
        frame = telem.copy()
        for tile in tiles:
            if t >= tile["life"]:
                continue
            x = tile["x"] + tile["vx"] * t
            y = tile["y"] + tile["vy"] * t + 0.5 * GRAVITY * t * t
            if y > H or x < -TILE or x > W:
                continue
            blit_tile(frame, tile, int(round(x)), int(round(y)))
        frames.append(frame)

    frames.append(telem.copy())
    return frames


def save(frames, name):
    OUT.mkdir(exist_ok=True)
    imgs = []
    for f in frames:
        rgb = np.zeros((H, W, 3), np.uint8)
        rgb[f] = (200, 230, 255)
        imgs.append(Image.fromarray(rgb).resize((W * SCALE, H * SCALE), Image.NEAREST))
    imgs[0].save(OUT / name, save_all=True, append_images=imgs[1:], duration=STEP_MS, loop=0)
    return imgs


if __name__ == "__main__":
    fr = render(mock_telemetry())
    print("кадров:", len(fr), "длительность, с:", len(fr) * STEP_MS / 1000)
    imgs = save(fr, "break.gif")
    imgs[LOGO_HOLD + 12].save(OUT / "break_a.png")
    imgs[LOGO_HOLD + 19].save(OUT / "break_b.png")
    imgs[LOGO_HOLD + 20 + 6].save(OUT / "break_c.png")
