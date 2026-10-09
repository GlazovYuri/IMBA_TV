"""Превью перехода «взрыв TNT из Minecraft» (частица explode из Java Edition 1.12).

Параметры взяты ПО ПАМЯТИ из кода игры (исходники открыть не удалось), см. константы ниже.
Тик игры = 50 мс, один шаг превью = один тик.
Запуск: .venv/bin/python preview_explosion.py  (результат: out/explosion.gif)
"""
import math
import random

import numpy as np
from PIL import Image

from preview import H, OUT, W, load_logo, load_sprites, mock_telemetry
from preview_wave import SCALE, dilate

TICK_MS = 50
LOGO_HOLD_TICKS = 20          # 1 с
TOTAL_TICKS = 44              # длительность взрыва
SEED = 21

# --- параметры оригинала (по памяти) ---
EXPLOSION_SIZE = 4.0          # TNT
PX_PER_BLOCK = 14.0           # масштаб: блок -> пиксели экрана
FRICTION = 0.9                # motion *= 0.9 каждый тик
RISE = 0.004                  # motionY += 0.004 (всплывает)
PARTICLES = 170               # частиц explode на взрыв
MAX_AGE_CAP = 40              # ограничение времени жизни (в игре до 82 тиков)
REVEAL_PX_PER_TICK = 16       # радиус, в котором логотип уже заменён интерфейсом

SPR = load_sprites()          # 8 кадров 8x8 (bool): 7 = самый большой, 0 = точка


def make_particles(rng):
    ps = []
    while len(ps) < PARTICLES:
        # точка внутри сферы радиуса size (разрушенный блок)
        x, y, z = (rng.uniform(-1, 1) * EXPLOSION_SIZE for _ in range(3))
        d = math.sqrt(x * x + y * y + z * z)
        if d > EXPLOSION_SIZE or d < 1e-3:
            continue
        x, y, z = x + 0.0, y, z
        dx, dy, dz = x / d, y / d, z / d
        # скорость: 0.5 / (dist/size + 0.1) * (rand*rand + 0.3), блоков/тик
        sp = 0.5 / (d / EXPLOSION_SIZE + 0.1) * (rng.random() * rng.random() + 0.3)
        j = lambda: (rng.random() * 2 - 1) * 0.05
        max_age = int(16.0 / (rng.random() * 0.8 + 0.2)) + 2
        ps.append(dict(
            x=x / 2, y=y / 2, z=z / 2,                    # спавн на полпути до центра
            vx=dx * sp + j(), vy=dy * sp + j(),
            scale=rng.random() * rng.random() * 6 + 1,
            max_age=min(max_age, MAX_AGE_CAP),
        ))
    ps.sort(key=lambda p: p["z"])                         # дальние рисуются первыми
    return ps


def draw_particle(frame, p, age):
    k = 7 - age * 8 // p["max_age"]
    if k < 0:
        return
    size = max(2, int(round(0.2 * p["scale"] * PX_PER_BLOCK)))
    spr = np.array(Image.fromarray(SPR[k].astype(np.uint8) * 255).resize((size, size), Image.NEAREST)) > 0
    cx = W / 2 + p["x"] * PX_PER_BLOCK
    cy = H / 2 + p["y"] * PX_PER_BLOCK
    x0, y0 = int(round(cx - size / 2)), int(round(cy - size / 2))
    # рамка: чёрный ободок вокруг пикселей частицы, затем белые пиксели
    white = np.pad(spr, 1)
    pad = np.pad(spr, 2)
    halo = np.any([pad[dy:dy + size + 2, dx:dx + size + 2] for dy in range(3) for dx in range(3)], axis=0)
    for yy in range(size + 2):
        for xx in range(size + 2):
            X, Y = x0 - 1 + xx, y0 - 1 + yy
            if 0 <= X < W and 0 <= Y < H:
                if white[yy, xx]:
                    frame[Y, X] = True
                elif halo[yy, xx]:
                    frame[Y, X] = False


def render(telem):
    rng = random.Random(SEED)
    logo = load_logo()
    ps = make_particles(rng)
    ys, xs = np.mgrid[0:H, 0:W]
    dist = np.hypot(xs - W / 2, (ys - H / 2) * 1.0)

    frames = [logo.copy() for _ in range(LOGO_HOLD_TICKS)]
    for t in range(TOTAL_TICKS):
        base = np.where(dist < REVEAL_PX_PER_TICK * (t + 1), telem, logo)
        frame = base.copy()
        for p in ps:
            if t >= p["max_age"]:
                continue
            # позиция после t тиков
            x, y, vx, vy = p["x"], p["y"], p["vx"], p["vy"]
            for _ in range(t):
                x += vx; y += vy
                vx *= FRICTION; vy = vy * FRICTION - RISE
            q = dict(p, x=x, y=y)
            draw_particle(frame, q, t)
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
    imgs[0].save(OUT / name, save_all=True, append_images=imgs[1:], duration=TICK_MS, loop=0)
    return imgs


if __name__ == "__main__":
    fr = render(mock_telemetry())
    print("кадров:", len(fr), "длительность, с:", len(fr) * TICK_MS / 1000)
    imgs = save(fr, "explosion.gif")
    for i, n in [(LOGO_HOLD_TICKS + 1, "a"), (LOGO_HOLD_TICKS + 4, "b"), (LOGO_HOLD_TICKS + 12, "c")]:
        imgs[i].save(OUT / f"expl_{n}.png")
