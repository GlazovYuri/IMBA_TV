"""Эталонные данные для тестов web/test, полученные через adafruit-nrfutil.

Запуск: pip install adafruit-nrfutil intelhex && python3 generate.py
"""
import json
import os
import struct
import subprocess
import zipfile

from intelhex import IntelHex
from nordicsemi.dfu import dfu_transport_serial as t

here = os.path.dirname(os.path.abspath(__file__))

# Образ с таблицей векторов (SP в RAM, Reset внутри приложения), пропуском между
# сегментами, длиной не кратной 4 и записью в UICR, которую nrfutil отбрасывает.
ih = IntelHex()
head = struct.pack('<II', 0x20040000, 0x26000 + 0x1D5)
seg1 = head + bytes((i * 7 + 3) & 0xFF for i in range(2000 - len(head)))
seg2 = bytes((i * 13 + 0xC0) & 0xFF for i in range(1001))
for i, b in enumerate(seg1):
    ih[0x26000 + i] = b
for i, b in enumerate(seg2):
    ih[0x26900 + i] = b
ih[0x10001014] = 0x00
ih.write_hex_file(os.path.join(here, 'app.hex'))

subprocess.run(['adafruit-nrfutil', 'dfu', 'genpkg', '--dev-type', '0x0052', '--sd-req', '0xFFFE',
                '--application', os.path.join(here, 'app.hex'), os.path.join(here, 'app.zip')], check=True)

# Тот же пакет, но сжатый и вложенный в папку — так бывает, если архив пересобрать вручную
with zipfile.ZipFile(os.path.join(here, 'app.zip')) as src, \
        zipfile.ZipFile(os.path.join(here, 'app-deflate.zip'), 'w', zipfile.ZIP_DEFLATED) as dst:
    for name in src.namelist():
        dst.writestr('pkg/' + name, src.read(name))

# HCI-кадры, которые nrfutil отправляет в порт
t.HciPacket.sequence_number = 0
frames = []
for payload in [
    t.int32_to_bytes(t.DFU_START_PACKET) + t.int32_to_bytes(4) + t.int32_to_bytes(0) * 2 + t.int32_to_bytes(3004),
    t.int32_to_bytes(t.DFU_DATA_PACKET) + ''.join(chr(c) for c in [0xC0, 0xDB, 0x00, 0xDC, 0xDD, 0xFF] * 100),
    t.int32_to_bytes(t.DFU_STOP_DATA_PACKET),
]:
    pkt = t.HciPacket([c for c in payload])
    frames.append({'payload': [ord(c) for c in payload], 'frame': pkt.data})

with open(os.path.join(here, 'hci_frames.json'), 'w') as f:
    json.dump(frames, f)
