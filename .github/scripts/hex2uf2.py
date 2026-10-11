#!/usr/bin/env python3
"""Конвертирует Intel HEX в UF2 для загрузчика Adafruit nRF52 (семейство nRF52840).

UF2-файл копируется на диск, который появляется после двойного нажатия reset.
Использование: hex2uf2.py input.hex output.uf2
"""
import struct
import sys

UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30
UF2_FLAG_FAMILY_ID = 0x00002000
FAMILY_NRF52840 = 0xADA52840
PAYLOAD = 256
UICR_START = 0x10000000


def read_hex(path):
    data = {}
    base = 0
    with open(path) as f:
        for n, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue
            if not line.startswith(':'):
                raise ValueError(f'{path}:{n}: not an Intel HEX record')
            rec = bytes.fromhex(line[1:])
            if sum(rec) & 0xFF:
                raise ValueError(f'{path}:{n}: bad checksum')
            length, addr, kind = rec[0], (rec[1] << 8) | rec[2], rec[3]
            payload = rec[4:4 + length]
            if kind == 0x00:
                for i, b in enumerate(payload):
                    data[base + addr + i] = b
            elif kind == 0x01:
                break
            elif kind == 0x02:
                base = int.from_bytes(payload, 'big') * 16
            elif kind == 0x04:
                base = int.from_bytes(payload, 'big') << 16
    # UICR не входит в образ приложения
    return {a: b for a, b in data.items() if a < UICR_START}


def to_uf2(data):
    pages = sorted({a & ~(PAYLOAD - 1) for a in data})
    out = bytearray()
    for n, page in enumerate(pages):
        chunk = bytes(data.get(page + i, 0xFF) for i in range(PAYLOAD))
        block = struct.pack('<8I', UF2_MAGIC_START0, UF2_MAGIC_START1, UF2_FLAG_FAMILY_ID,
                            page, PAYLOAD, n, len(pages), FAMILY_NRF52840)
        block += chunk
        block += bytes(512 - 4 - len(block))
        block += struct.pack('<I', UF2_MAGIC_END)
        out += block
    return bytes(out)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    data = read_hex(sys.argv[1])
    if not data:
        sys.exit('no data in hex file')
    with open(sys.argv[2], 'wb') as f:
        f.write(to_uf2(data))


if __name__ == '__main__':
    main()
