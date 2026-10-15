#pragma once
#include <stdint.h>

// Снимок LoEUC из характеристики ...2c07: раз в секунду все метрики колеса и метка времени.
// Формат: https://loeuc.ru/ru/docs/devices («Если экран умеет больше»)

// кусок снимка, как его записал телефон; вызывается из обработчика записи BLE
void snapshotChunk(const uint8_t* data, uint16_t len);

// Unix-время сейчас в мс: метка последнего снимка плюс millis() с его прихода.
// false - времени нет: снимков с включения не было или метка не похожа на Unix-время
bool snapshotTimeMs(uint64_t& unix_ms);

// биты present у зарядки: какие величины были в последнем снимке
enum
{
  SNAP_HAS_CURRENT = 1 << 0,
  SNAP_HAS_BATTERY_TEMP = 1 << 1,
  SNAP_HAS_CHARGER_TEMP = 1 << 2,
};

// зарядка колеса по последнему снимку
struct snapshot_charge_t
{
  uint8_t present;
  uint16_t current_x10;      // ток зарядки, А * 10
  int16_t battery_temp_x10;  // самая горячая точка батареи, °C * 10
  int16_t charger_temp_x10;  // зарядник, °C * 10
};

// true - колесо заряжается: так сказал свежий снимок. Снимок старше пары секунд не считается,
// LoEUC шлёт его раз в секунду, и без связи зарядка на экране не должна залипнуть
bool snapshotCharge(snapshot_charge_t& charge);
