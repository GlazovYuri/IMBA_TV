"""Превью перехода «волна с пузырями» (как смена сцен в «Губке Бобе»).

Логотип -> волнистая кромка воды идёт слева направо, за ней проявляется телеметрия,
на кромке рождаются пузыри, всплывают и лопаются.
Запуск: .venv/bin/python preview_wave.py  (результат: out/wave.gif)
"""
import math
import random

import numpy as np
from PIL import Image, ImageDraw

from preview import H, OUT, STEP_MS, W, load_logo, mock_telemetry

LOGO_HOLD = 25
SWEEP_STEPS = 28
TAIL_STEPS = 18          # время, за которое доживают последние пузыри
SEED = 3
SCALE = 6


def edge_x(y: np.ndarray, t: int) -> np.ndarray:
    """x кромки воды для каждой строки y на шаге t (с ease-in-out)."""
    p = min(t / SWEEP_STEPS, 1.0)
    p = p * p * (3 - 2 * p)
    base = -14 + p * (W + 28)
    wob = 6 * np.sin(y * 0.2 + t * 0.5) + 1.5 * np.sin(y * 0.45 - t * 0.8)
    return base + wob


def make_bubbles(rng):
    bubbles = []
    cols, rows = 9, 4                     # сетка с jitter: равномерное покрытие экрана
    cells = [(c, r) for c in range(cols) for r in range(rows)]
    for c, r in cells:
        bx = (c + rng.uniform(0.15, 0.85)) * W / cols
        by = (r + rng.uniform(0.3, 1.0)) * H / rows
        # рождается там, где кромка проходит над точкой (x, y)
        # обратная функция ease: подбираем шаг t, когда кромка достигает bx
        t_born = min(range(SWEEP_STEPS + 1), key=lambda t: abs(float(edge_x(np.array([by]), t)[0]) - bx))
        bubbles.append(dict(
            x=bx, y=by, r=rng.choice([3, 4, 5, 6, 7]),
            born=t_born + rng.randint(0, 3), life=rng.randint(10, 18),
            drift=rng.uniform(-0.25, 0.25), rise=rng.uniform(0.7, 1.4), ph=rng.uniform(0, 6.28),
        ))
    return bubbles


def bubble_layer(bubbles, t) -> np.ndarray:
    im = Image.new("1", (W, H), 0)
    d = ImageDraw.Draw(im)
    for b in bubbles:
        age = t - b["born"]
        if not 0 <= age < b["life"]:
            continue
        cx = b["x"] + b["drift"] * age + 1.2 * math.sin(age * 0.6 + b["ph"])
        cy = b["y"] - b["rise"] * age
        r = b["r"] + age * 0.08
        if age >= b["life"] - 2:  # лопается: разлетаются точки
            k = age - (b["life"] - 3)
            for a in range(0, 360, 60):
                px = cx + (r + k) * math.cos(math.radians(a))
                py = cy + (r + k) * math.sin(math.radians(a))
                d.point((round(px), round(py)), fill=1)
            continue
        d.ellipse([cx - r, cy - r, cx + r, cy + r], outline=1)
        if r >= 3:  # блик
            d.point((round(cx - r * 0.45), round(cy - r * 0.45)), fill=1)
    return np.array(im, bool)


def dilate(m: np.ndarray) -> np.ndarray:
    p = np.pad(m, 1)
    return np.any([p[dy:dy + H, dx:dx + W] for dy in range(3) for dx in range(3)], axis=0)


def render(telem):
    rng = random.Random(SEED)
    logo = load_logo()
    bubbles = make_bubbles(rng)
    ys = np.arange(H)[:, None]
    xs = np.arange(W)[None, :]
    frames = [logo.copy() for _ in range(LOGO_HOLD)]

    for t in range(SWEEP_STEPS + TAIL_STEPS):
        ex = edge_x(ys[:, 0], t)[:, None]
        behind = xs < ex                      # там, где вода уже прошла
        base = np.where(behind, telem, logo)
        # кромка: светлая линия 2 px с чёрной подложкой справа, чтобы читалась на логотипе
        line = (np.abs(xs - ex) < 1.3)
        halo = (xs >= ex + 1.0) & (xs < ex + 2.6)
        if t > SWEEP_STEPS:
            line = line & False
            halo = halo & False
        frame = np.where(halo, False, base) | line
        # пузыри: белое кольцо с чёрным ободком (на устройстве XOR недоступен: буфер OLED нельзя читать)
        ring = bubble_layer(bubbles, t)
        frame = np.where(dilate(ring), False, frame) | ring
        frames.append(frame)

    frames.append(telem.copy())
    return frames


def save_gif(frames, name):
    OUT.mkdir(exist_ok=True)
    imgs = []
    for f in frames:
        rgb = np.zeros((H, W, 3), np.uint8)
        rgb[f] = (200, 230, 255)
        imgs.append(Image.fromarray(rgb).resize((W * SCALE, H * SCALE), Image.NEAREST))
    imgs[0].save(OUT / name, save_all=True, append_images=imgs[1:], duration=STEP_MS, loop=0)
    imgs[LOGO_HOLD + 8].save(OUT / "wave1.png")
    imgs[LOGO_HOLD + 16].save(OUT / "wave2.png")
    imgs[LOGO_HOLD + 24].save(OUT / "wave3.png")


if __name__ == "__main__":
    for name, kw in [("wave.gif", {})]:
        fr = render(mock_telemetry(**kw))
        print(name, "кадров:", len(fr), "длительность, с:", len(fr) * STEP_MS / 1000)
        save_gif(fr, name)
