#pragma once
#include <stdint.h>

// QR-код со ссылкой на сайт для экрана обновления по Bluetooth: светлый квадрат
// во всю высоту экрана, модули по 2 px. Массив генерирует tools/gen_qr.py
const int qr_size = 64;

// определение в qr.cpp, чтобы массив был во флеше в одном экземпляре
extern const uint8_t qr_bitmap[qr_size * qr_size / 8];
