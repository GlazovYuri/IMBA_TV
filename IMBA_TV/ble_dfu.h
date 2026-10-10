#pragma once
#include <bluefruit.h>

// Сервис обновления по Bluetooth вместо BLEDfu из ядра: те же UUID и протокол, сайт разницы не видит.
// Отличие одно: данные о телефоне для загрузчика записываются в порядке, который ждёт его версия,
// иначе загрузчик 0.10.0 и новее их отвергает и после перехода рекламируется с другим адресом
class BleDfuService : public BLEService
{
public:
  BleDfuService();
  err_t begin() override;

private:
  BLECharacteristic _chr_control;
};
