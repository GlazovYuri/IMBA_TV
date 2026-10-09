#!/usr/bin/env python3
"""Генерирует IMBA_TV/qr.cpp: QR-код со ссылкой на сайт для экрана обновления по Bluetooth.

Код занимает всю высоту экрана: светлый квадрат 64×64 с тёмными модулями по 2×2 пикселя.
Так помещается QR версии 3 (29×29 модулей), а в неё с уровнем коррекции M входит ссылка
до 42 байт. Модули тёмные на светлом, потому что инверсные коды читают не все сканеры.
После смены ссылки запустите:

    python3 tools/gen_qr.py [ссылка]
"""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TARGET = os.path.join(ROOT, 'IMBA_TV', 'qr.cpp')
URL = 'https://glazovyuri.github.io/IMBA_TV/'

IMAGE_SIZE = 64  # px, как qr_size в qr.h
MODULE_SIZE = 2  # px

# Уровень коррекции M, один блок: (всего кодовых слов, из них коррекции) для версий 1-3.
# Версия 4 (33 модуля) в 64 px уже не помещается
VERSIONS = {1: (26, 10), 2: (44, 16), 3: (70, 26)}
FORMAT_ECC_M = 0b00

MASKS = [
    lambda x, y: (x + y) % 2 == 0,
    lambda x, y: y % 2 == 0,
    lambda x, y: x % 3 == 0,
    lambda x, y: (x + y) % 3 == 0,
    lambda x, y: (x // 3 + y // 2) % 2 == 0,
    lambda x, y: x * y % 2 + x * y % 3 == 0,
    lambda x, y: (x * y % 2 + x * y % 3) % 2 == 0,
    lambda x, y: ((x + y) % 2 + x * y % 3) % 2 == 0,
]

# 1:1:3:1:1 с четырьмя светлыми модулями сбоку, похоже на поисковый узор (штраф N3)
FINDER_LIKE = [True, False, True, True, True, False, True, False, False, False, False]


def gf_mul(x, y):
    z = 0
    for i in reversed(range(8)):
        z = (z << 1) ^ ((z >> 7) * 0x11D)
        z ^= ((y >> i) & 1) * x
    return z


def rs_remainder(data, degree):
    # делитель (x - a^0)(x - a^1)...(x - a^(degree-1)) без старшего коэффициента
    divisor = [0] * (degree - 1) + [1]
    root = 1
    for _ in range(degree):
        for j in range(degree):
            divisor[j] = gf_mul(divisor[j], root)
            if j + 1 < degree:
                divisor[j] ^= divisor[j + 1]
        root = gf_mul(root, 0x02)

    result = [0] * degree
    for b in data:
        factor = b ^ result.pop(0)
        result.append(0)
        for i, coef in enumerate(divisor):
            result[i] ^= gf_mul(coef, factor)
    return result


def encode_data(payload, data_len):
    bits = []

    def put(value, n):
        bits.extend((value >> i) & 1 for i in reversed(range(n)))

    put(0b0100, 4)  # байтовый режим
    put(len(payload), 8)
    for b in payload:
        put(b, 8)
    capacity = data_len * 8
    put(0, min(4, capacity - len(bits)))
    put(0, -len(bits) % 8)

    codewords = [int(''.join(map(str, bits[i:i + 8])), 2) for i in range(0, len(bits), 8)]
    pad = 0xEC
    while len(codewords) < data_len:
        codewords.append(pad)
        pad ^= 0xEC ^ 0x11
    return codewords


def draw_format(fn, size, mask):
    data = FORMAT_ECC_M << 3 | mask
    rem = data
    for _ in range(10):
        rem = (rem << 1) ^ ((rem >> 9) * 0x537)
    bits = (data << 10 | rem) ^ 0x5412

    def bit(i):
        return (bits >> i) & 1 == 1

    for i in range(6):
        fn(8, i, bit(i))
    fn(8, 7, bit(6))
    fn(8, 8, bit(7))
    fn(7, 8, bit(8))
    for i in range(9, 15):
        fn(14 - i, 8, bit(i))
    for i in range(8):
        fn(size - 1 - i, 8, bit(i))
    for i in range(8, 15):
        fn(8, size - 15 + i, bit(i))
    fn(8, size - 8, True)


def build(version, codewords, mask):
    size = version * 4 + 17
    dark = [[False] * size for _ in range(size)]
    func = [[False] * size for _ in range(size)]

    def fn(x, y, value):
        dark[y][x] = value
        func[y][x] = True

    for i in range(size):
        fn(6, i, i % 2 == 0)
        fn(i, 6, i % 2 == 0)
    for cx, cy in ((3, 3), (size - 4, 3), (3, size - 4)):
        for dy in range(-4, 5):
            for dx in range(-4, 5):
                x, y = cx + dx, cy + dy
                if 0 <= x < size and 0 <= y < size:
                    fn(x, y, max(abs(dx), abs(dy)) not in (2, 4))
    if version >= 2:
        c = size - 7
        for dy in range(-2, 3):
            for dx in range(-2, 3):
                fn(c + dx, c + dy, max(abs(dx), abs(dy)) != 1)
    draw_format(fn, size, mask)

    # данные змейкой по парам столбцов, справа налево, столбец 6 занят разметкой
    bits = [(cw >> (7 - i)) & 1 for cw in codewords for i in range(8)]
    i = 0
    right = size - 1
    while right >= 1:
        if right == 6:
            right = 5
        upward = ((right + 1) & 2) == 0
        for vert in range(size):
            y = size - 1 - vert if upward else vert
            for x in (right, right - 1):
                if not func[y][x] and i < len(bits):
                    dark[y][x] = bits[i] == 1
                    i += 1
        right -= 2

    for y in range(size):
        for x in range(size):
            if not func[y][x] and MASKS[mask](x, y):
                dark[y][x] = not dark[y][x]
    return dark


def penalty(m):
    size = len(m)
    score = 0
    for line in m + [list(col) for col in zip(*m)]:
        run = 1
        for a, b in zip(line, line[1:]):
            if a == b:
                run += 1
            else:
                if run >= 5:
                    score += run - 2
                run = 1
        if run >= 5:
            score += run - 2

        padded = [False] * 4 + line + [False] * 4
        for k in range(len(padded) - 10):
            window = padded[k:k + 11]
            if window == FINDER_LIKE or window == FINDER_LIKE[::-1]:
                score += 40

    for y in range(size - 1):
        for x in range(size - 1):
            if m[y][x] == m[y][x + 1] == m[y + 1][x] == m[y + 1][x + 1]:
                score += 3

    dark = sum(map(sum, m))
    total = size * size
    score += ((abs(dark * 20 - total * 10) + total - 1) // total - 1) * 10
    return score


def make(text):
    payload = text.encode('utf-8')
    for version, (total, ecc) in VERSIONS.items():
        if len(payload) <= total - ecc - 2:
            break
    else:
        sys.exit(f'Ссылка длиннее {VERSIONS[3][0] - VERSIONS[3][1] - 2} байт: QR не поместится на экран')

    data = encode_data(payload, total - ecc)
    codewords = data + rs_remainder(data, ecc)
    mask = min(range(8), key=lambda k: penalty(build(version, codewords, k)))
    return version, mask, build(version, codewords, mask)


def render(modules):
    lit = [[True] * IMAGE_SIZE for _ in range(IMAGE_SIZE)]
    offset = (IMAGE_SIZE - len(modules) * MODULE_SIZE) // 2
    for y, row in enumerate(modules):
        for x, dark in enumerate(row):
            if not dark:
                continue
            for dy in range(MODULE_SIZE):
                for dx in range(MODULE_SIZE):
                    lit[offset + y * MODULE_SIZE + dy][offset + x * MODULE_SIZE + dx] = False

    # формат GyverOLED drawBitmap: строки по 8 px сверху вниз, в байте младший бит сверху
    return [sum(lit[page * 8 + bit][x] << bit for bit in range(8))
            for page in range(IMAGE_SIZE // 8) for x in range(IMAGE_SIZE)]


def main():
    url = sys.argv[1] if len(sys.argv) > 1 else URL
    version, mask, modules = make(url)
    bitmap = render(modules)

    rows = [', '.join(f'0x{b:02x}' for b in bitmap[i:i + 16]) for i in range(0, len(bitmap), 16)]
    body = ',\n  '.join(rows)
    with open(TARGET, 'w', encoding='utf-8', newline='\n') as f:
        f.write(f'// Сгенерировано tools/gen_qr.py, не редактировать вручную.\n'
                f'// {url}: QR версии {version}, коррекция M, маска {mask}\n'
                f'#include "qr.h"\n\n'
                f'const uint8_t qr_bitmap[qr_size * qr_size / 8] = {{\n  {body}\n}};\n')
    print(f'{os.path.relpath(TARGET, ROOT)}: {url}, версия {version}, маска {mask}')


if __name__ == '__main__':
    main()
