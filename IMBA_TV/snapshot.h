#pragma once
#include <stdint.h>

// Снимок LoEUC из характеристики ...2c07: раз в секунду все метрики колеса и метка времени.
// Формат: https://loeuc.ru/ru/docs/devices («Если экран умеет больше»)

// кусок снимка, как его записал телефон; вызывается из обработчика записи BLE
void snapshotChunk(const uint8_t* data, uint16_t len);

// Unix-время сейчас в мс: метка последнего снимка плюс millis() с его прихода.
// false - времени нет: снимков с включения не было или метка не похожа на Unix-время
bool snapshotTimeMs(uint64_t& unix_ms);
