#!/usr/bin/env python3
"""Снимает превью анимаций дисплея для сайта: web/previews/<имя>.gif и .png.

Код экранов и анимаций прошивки собирается на компьютере вместе с библиотекой GyverOLED
(tools/previews/harness.cpp): вывод на экран идёт в эмулятор контроллера SSH1106, время
виртуальное и течёт со скоростью шины I2C, поэтому кадры и их длительность как на дисплее.
PNG - один кадр для тех, у кого в системе отключена анимация.

Нужны компилятор C++ и библиотека GyverOLED (та же, что для Arduino IDE).
После изменения анимаций или основного экрана запустите:

    python3 tools/render_previews.py [--gyveroled ПУТЬ_К_GyverOLED/src]
"""
import argparse
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKETCH = os.path.join(ROOT, 'IMBA_TV')
HARNESS = os.path.join(ROOT, 'tools', 'previews')
OUT = os.path.join(ROOT, 'web', 'previews')
SOURCES = ['animations', 'display', 'screen_main', 'screen_grid', 'screen_info', 'screen_text', 'config', 'logo', 'qr']

W, H = 128, 64
FRAME_BYTES = W * H // 8
PIXEL_ON = (238, 240, 242)
PIXEL_OFF = (16, 16, 16)
SEED = 0x5EED1234

# кадр для PNG: момент от начала записи, мс (у заставок первая секунда - логотип)
POSTER_MS = {'logo': 500, 'wave': 1500, 'break': 1650, 'explosion': 1500,
             'clawd_type': 3000, 'clawd_laptop': 3050, 'clawd_pixels': 2900, 'poweroff': 1330}


def find_gyveroled():
    candidates = [os.path.expanduser('~/Documents/Arduino/libraries/GyverOLED/src'),
                  os.path.expanduser('~/Arduino/libraries/GyverOLED/src')]
    for path in candidates:
        if os.path.exists(os.path.join(path, 'GyverOLED.h')):
            return path
    sys.exit('Не найдена библиотека GyverOLED, укажите --gyveroled путь/к/GyverOLED/src')


def build(gyveroled, workdir):
    exe = os.path.join(workdir, 'harness')
    cxx = os.environ.get('CXX', 'c++')
    cmd = [cxx, '-std=c++17', '-O2', '-w',
           '-I', os.path.join(HARNESS, 'shim'), '-I', gyveroled, '-I', SKETCH,
           os.path.join(HARNESS, 'harness.cpp'), *[os.path.join(SKETCH, f'{s}.cpp') for s in SOURCES],
           '-o', exe]
    subprocess.run(cmd, check=True)
    return exe


def read_frames(path):
    """[(время мс, контраст, инверсия, пиксели)], время конца записи."""
    data = open(path, 'rb').read()
    frames = []
    pos = 0
    record = 4 + 2 + FRAME_BYTES
    while len(data) - pos >= record:
        t, contrast, inverted = struct.unpack_from('<IBB', data, pos)
        frames.append((t, contrast, bool(inverted), data[pos + 6:pos + record]))
        pos += record
    (end,) = struct.unpack_from('<I', data, pos)
    return frames, end


def frame_colors(contrast, inverted):
    # яркость OLED растёт с контрастом, но и при нуле экран не гаснет полностью
    k = 0.15 + 0.85 * contrast / 255
    on = tuple(round(c * k) for c in PIXEL_ON)
    return (on, PIXEL_OFF) if not inverted else (PIXEL_OFF, on)


def to_image(frame):
    """Кадр в RGB-тройки по строкам."""
    _, contrast, inverted, pixels = frame
    on, off = frame_colors(contrast, inverted)
    rows = []
    for y in range(H):
        row = []
        for x in range(W):
            bit = pixels[(y * W + x) >> 3] >> (7 - (x & 7)) & 1
            row.append(on if bit else off)
        rows.append(row)
    return rows


def timeline(frames, end):
    """Склеивает одинаковые кадры, переводит длительности в сотые доли секунды.

    Кадры короче 20 мс браузеры показывают по 100 мс, поэтому они отдают время следующему."""
    start = frames[0][0]
    marks = [f[0] - start for f in frames] + [end - start]
    out = []
    for i, frame in enumerate(frames):
        key = frame[1:]
        cs_from, cs_to = round(marks[i] / 10), round(marks[i + 1] / 10)
        if out and out[-1][0][1:] == key:
            out[-1][1] += cs_to - cs_from
            continue
        out.append([frame, cs_to - cs_from])
    merged = []
    carry = 0
    for frame, cs in out:
        cs += carry
        if cs < 2:
            carry = cs
            continue
        carry = 0
        merged.append((frame, cs))
    if carry and merged:
        merged[-1] = (merged[-1][0], merged[-1][1] + carry)
    return merged


def lzw(indices, min_code):
    clear = 1 << min_code
    eoi = clear + 1
    out = bytearray()
    bitbuf = 0
    nbits = 0
    code_size = min_code + 1

    def emit(code):
        nonlocal bitbuf, nbits
        bitbuf |= code << nbits
        nbits += code_size
        while nbits >= 8:
            out.append(bitbuf & 0xFF)
            bitbuf >>= 8
            nbits -= 8

    def reset():
        return {bytes([i]): i for i in range(clear)}, eoi + 1

    table, next_code = reset()
    emit(clear)
    word = b''
    for value in indices:
        candidate = word + bytes([value])
        if candidate in table:
            word = candidate
            continue
        emit(table[word])
        if next_code < 4096:
            # ширина кода растёт, как только новый код в неё не влезает
            if next_code >= (1 << code_size) and code_size < 12:
                code_size += 1
            table[candidate] = next_code
            next_code += 1
        else:
            emit(clear)
            table, next_code = reset()
            code_size = min_code + 1
        word = bytes([value])
    emit(table[word])
    emit(eoi)
    if nbits:
        out.append(bitbuf & 0xFF)
    return bytes(out)


def write_gif(path, images):
    """images: [(строки RGB, длительность в сотых)], бесконечный повтор."""
    palette = []
    for rows, _ in images:
        for row in rows:
            for color in row:
                if color not in palette:
                    palette.append(color)
    bits = max(1, (len(palette) - 1).bit_length())
    size = 1 << bits
    lookup = {c: i for i, c in enumerate(palette)}
    min_code = max(2, bits)

    out = bytearray(b'GIF89a')
    out += struct.pack('<HHBBB', W, H, 0x80 | ((bits - 1) << 4) | (bits - 1), 0, 0)
    for color in palette + [(0, 0, 0)] * (size - len(palette)):
        out += bytes(color)
    out += b'\x21\xFF\x0BNETSCAPE2.0\x03\x01\x00\x00\x00'
    for rows, cs in images:
        out += b'\x21\xF9\x04\x04' + struct.pack('<H', cs) + b'\x00\x00'
        out += b'\x2C' + struct.pack('<HHHHB', 0, 0, W, H, 0)
        data = lzw(bytes(lookup[c] for row in rows for c in row), min_code)
        out.append(min_code)
        for i in range(0, len(data), 255):
            chunk = data[i:i + 255]
            out.append(len(chunk))
            out += chunk
        out.append(0)
    out.append(0x3B)
    with open(path, 'wb') as f:
        f.write(out)


def write_png(path, rows):
    raw = b''.join(b'\x00' + b''.join(bytes(c) for c in row) for row in rows)

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
    png = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', W, H, 8, 2, 0, 0, 0))
           + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))
    with open(path, 'wb') as f:
        f.write(png)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--gyveroled', help='путь к GyverOLED/src')
    parser.add_argument('--keep', help='сохранить сырые кадры в эту папку')
    args = parser.parse_args()

    gyveroled = args.gyveroled or find_gyveroled()
    os.makedirs(OUT, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        exe = build(gyveroled, tmp)
        subprocess.run([exe, tmp, hex(SEED)], check=True)
        for name, poster_ms in POSTER_MS.items():
            frames, end = read_frames(os.path.join(tmp, f'{name}.bin'))
            images = [(to_image(frame), cs) for frame, cs in timeline(frames, end)]
            write_gif(os.path.join(OUT, f'{name}.gif'), images)

            start = frames[0][0]
            poster = [f for f in frames if f[0] - start <= poster_ms][-1]
            write_png(os.path.join(OUT, f'{name}.png'), to_image(poster))
            total = sum(cs for _, cs in images) * 10
            size = os.path.getsize(os.path.join(OUT, f'{name}.gif'))
            print(f'{name}: {len(images)} кадров, {total} мс, {size // 1024} КБ')
        if args.keep:
            os.makedirs(args.keep, exist_ok=True)
            for name in POSTER_MS:
                shutil.copy(os.path.join(tmp, f'{name}.bin'), args.keep)


if __name__ == '__main__':
    main()
