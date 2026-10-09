"""Собирает GIF из кадров, снятых с настоящего кода прошивки (host/intro_harness)."""
import sys
import numpy as np
from PIL import Image
from preview import H, OUT, W, mock_telemetry

NAMES = {0: ("wave", 40), 1: ("break", 40), 2: ("explosion", 50)}


def load(path):
    buf = open(path, "rb").read()
    frames = []
    for i in range(len(buf) // 1024):
        b = buf[i * 1024:(i + 1) * 1024]
        img = np.zeros((H, W), bool)
        for x in range(W):
            for y in range(H):
                img[y, x] = bool(b[(y >> 3) + x * 8] >> (y & 7) & 1)
        frames.append(img)
    return frames


def to_img(f, scale=6):
    rgb = np.zeros((H, W, 3), np.uint8)
    rgb[f] = (200, 230, 255)
    return Image.fromarray(rgb).resize((W * scale, H * scale), Image.NEAREST)


if __name__ == "__main__":
    final = mock_telemetry()
    for a, (name, step) in NAMES.items():
        fr = load(OUT / f"fw_anim{a}.bin") + [final]
        imgs = [to_img(f) for f in fr]
        durs = [1000] + [step] * (len(fr) - 2) + [800]
        imgs[0].save(OUT / f"fw_{name}.gif", save_all=True, append_images=imgs[1:], duration=durs, loop=0)
        # контрольные кадры
        n = len(fr)
        sheet = Image.new("RGB", (W * 3 * 2 + 20, H * 2 * 2 + 10), (40, 40, 40))
        picks = [n // 6, n // 3, n // 2, 2 * n // 3, 5 * n // 6, n - 2]
        for i, k in enumerate(picks):
            sheet.paste(to_img(fr[k], 2), ((i % 3) * (W * 2 + 10), (i // 3) * (H * 2 + 10)))
        sheet.save(OUT / f"fw_{name}_sheet.png")
        print(name, len(fr), "кадров")
