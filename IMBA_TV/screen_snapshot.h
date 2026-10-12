#pragma once
#include <stdint.h>

// Отладочный экран снимка LoEUC из характеристики ...2c07: метка времени, метрики и уход часов
// дисплея относительно телефона. Формат: https://loeuc.ru/ru/docs/devices («Если экран умеет больше»)

// кусок снимка, как его записал телефон; вызывается из обработчика записи BLE
void snapshotChunk(const uint8_t* data, uint16_t len);

void screenSnapshotDraw();
