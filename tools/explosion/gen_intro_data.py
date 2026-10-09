"""Генерирует IMBA_TV/intro_data.h: спрайты частиц и маски трещин в 1-битном виде.

Источники (не коммитятся): src_old/particles.png (Minecraft 1.12.2), src_destroy/destroy_stage_*.png (1.21.4).
Запуск: .venv/bin/python gen_intro_data.py
"""
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).parent
OUT = ROOT / "../../IMBA_TV/intro_data.h"

atlas = np.array(Image.open(ROOT / "src_old/particles.png").convert("RGBA"))
puff = [[0] * 8 for _ in range(8)]
for k in range(8):                       # первая строка атласа: 8 кадров 8x8
    for y in range(8):
        for x in range(8):
            if atlas[y, k * 8 + x, 3] > 0:
                puff[k][y] |= 1 << x

crack = []
for i in range(10):
    a = np.array(Image.open(ROOT / f"src_destroy/destroy_stage_{i}.png").convert("RGBA"))
    dark = (a[..., 3] == 255) & (a[..., 0] < 100)     # только тёмные пиксели трещин
    rows = []
    for y in range(16):
        r = 0
        for x in range(16):
            if dark[y, x]:
                r |= 1 << x
        rows.append(r)
    crack.append(rows)

lines = ["#pragma once", "#include <stdint.h>", "",
         "// Сгенерировано tools/explosion/gen_intro_data.py (не править руками).",
         "// Данные: 1-битные кадры, полученные из текстур Minecraft.", "",
         "// Частица взрыва: 8 кадров 8x8 (кадр 7 самый крупный, 0 - точка). Строка = байт, бит x = пиксель x.",
         "static const uint8_t mc_puff[8][8] = {"]
for k in range(8):
    lines.append("  {" + ", ".join(f"0x{v:02X}" for v in puff[k]) + "},")
lines += ["};", "",
          "// Трещины destroy_stage_0..9: 16 строк по 16 бит, бит x = пиксель x.",
          "static const uint16_t mc_crack[10][16] = {"]
for i in range(10):
    lines.append("  {" + ", ".join(f"0x{v:04X}" for v in crack[i]) + "},")
lines += ["};", ""]
OUT.write_text("\n".join(lines))
print("записано", OUT.resolve())
