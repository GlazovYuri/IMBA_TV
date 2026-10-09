"""Превью перехода «волна + частицы Minecraft»: как preview_wave.py, но вместо пузырей
пиксельные всплески (первая строка src_old/particles.png, 8 кадров 8x8).

Запуск: .venv/bin/python preview_wave_mc.py  (результат: out/wave_mc.gif)
"""
import random

import numpy as np
from PIL import Image

from preview import H, OUT, STEP_MS, W, load_sprites, mock_telemetry, load_logo
from preview_wave import (LOGO_HOLD, SWEEP_STEPS, TAIL_STEPS, dilate, edge_x, save_gif)

SEED = 5
SPRITES = load_sprites()  # 8 кадров 8x8 (bool)


def make_puffs(rng):
    puffs = []
    cols, rows = 8, 3
    for c in range(cols):
        for r in range(rows):
            bx = (c + rng.uniform(0.3, 0.7)) * W / cols
            by = (r + rng.uniform(0.3, 0.7)) * H / rows
            t_born = min(range(SWEEP_STEPS + 1), key=lambda t: abs(float(edge_x(np.array([by]), t)[0]) - bx))
            puffs.append(dict(
                x=bx, y=by, scale=rng.choice([2, 2, 3]),
                born=t_born + rng.randint(0, 3), step=rng.choice([1, 1, 2]),
                rise=rng.uniform(0.0, 0.5),
            ))
    return puffs


def puff_layer(puffs, t) -> np.ndarray:
    layer = np.zeros((H, W), bool)
    for p in puffs:
        k = (t - p["born"]) // p["step"]
        if not 0 <= k < 8:
            continue
        s = p["scale"]
        m = np.kron(SPRITES[k], np.ones((s, s), bool))
        size = 8 * s
        x0 = int(round(p["x"] - size / 2))
        y0 = int(round(p["y"] - size / 2 - p["rise"] * (t - p["born"])))
        for yy in range(size):
            for xx in range(size):
                X, Y = x0 + xx, y0 + yy
                if m[yy, xx] and 0 <= X < W and 0 <= Y < H:
                    layer[Y, X] = True
    return layer


def render(telem):
    rng = random.Random(SEED)
    logo = load_logo()
    puffs = make_puffs(rng)
    ys = np.arange(H)[:, None]
    xs = np.arange(W)[None, :]
    frames = [logo.copy() for _ in range(LOGO_HOLD)]

    for t in range(SWEEP_STEPS + TAIL_STEPS):
        ex = edge_x(ys[:, 0], t)[:, None]
        base = np.where(xs < ex, telem, logo)
        line = np.abs(xs - ex) < 1.3
        halo = (xs >= ex + 1.0) & (xs < ex + 2.6)
        if t > SWEEP_STEPS:
            line, halo = line & False, halo & False
        frame = np.where(halo, False, base) | line
        fx = puff_layer(puffs, t)
        frame = np.where(dilate(fx), False, frame) | fx   # белые частицы с чёрным ободком
        frames.append(frame)

    frames.append(telem.copy())
    return frames


if __name__ == "__main__":
    fr = render(mock_telemetry())
    print("кадров:", len(fr), "длительность, с:", len(fr) * STEP_MS / 1000)
    save_gif(fr, "wave_mc.gif")
