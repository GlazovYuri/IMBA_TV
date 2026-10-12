#pragma once
// Bluefruit (Adafruit nRF52) поверх модели SoftDevice эмулятора.
// Записываемые характеристики телефон находит по двум младшим байтам UUID (shared.h: kChar*).
#include <Arduino.h>
#include <nrf_soc.h>

#define CHR_PROPS_READ 0x02
#define CHR_PROPS_WRITE_WO_RESP 0x04
#define CHR_PROPS_WRITE 0x08
#define CHR_PROPS_NOTIFY 0x10

#define BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE 0x06

#define BLE_GATT_ATT_MTU_DEFAULT 23
#define BLE_GATT_ATT_MTU_MAX 247

enum
{
  BANDWIDTH_AUTO = 0,
  BANDWIDTH_LOW,
  BANDWIDTH_NORMAL,
  BANDWIDTH_HIGH,
  BANDWIDTH_MAX,
};

enum SecureMode_t
{
  SECMODE_NO_ACCESS = 0,
  SECMODE_OPEN = 0x11,
};

class BLECharacteristic;
typedef void (*ble_connect_cb_t)(uint16_t conn_hdl);
typedef void (*ble_disconnect_cb_t)(uint16_t conn_hdl, uint8_t reason);
typedef void (*ble_write_cb_t)(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len);

class BLEService
{
public:
  explicit BLEService(const uint8_t uuid128[16]);
  void begin();
  uint8_t uuid[16];
};

class BLECharacteristic
{
public:
  explicit BLECharacteristic(const uint8_t uuid128[16]);
  void setProperties(uint8_t props) { props_ = props; }
  void setPermission(SecureMode_t read, SecureMode_t write) { read_perm_ = read; write_perm_ = write; }
  void setFixedLen(uint16_t len) { fixed_len_ = len; max_len_ = len; }
  void setMaxLen(uint16_t len) { max_len_ = len; }
  void setWriteCallback(ble_write_cb_t cb) { write_cb_ = cb; }
  void begin();

  uint8_t uuid[16];
  uint8_t props_ = 0;
  SecureMode_t read_perm_ = SECMODE_OPEN;
  SecureMode_t write_perm_ = SECMODE_OPEN;
  uint16_t fixed_len_ = 0;
  uint16_t max_len_ = 20;
  ble_write_cb_t write_cb_ = nullptr;
};

class BLEDis
{
public:
  void setManufacturer(const char* s) { manufacturer = s; }
  void setModel(const char* s) { model = s; }
  void setHardwareRev(const char* s) { hw_rev = s; }
  void setFirmwareRev(const char* s) { fw_rev = s; }
  void setSoftwareRev(const char* s) { sw_rev = s; }
  void begin();

  const char* manufacturer = "";
  const char* model = "";
  const char* hw_rev = "";
  const char* fw_rev = "";
  const char* sw_rev = "";
};

// сервис обновления по воздуху: эмулятор его только объявляет, само обновление не моделируется
class BLEDfu : public BLEService
{
public:
  BLEDfu();
  void begin();
};

class BLEPeriph
{
public:
  void setConnIntervalMS(uint16_t min_ms, uint16_t max_ms);
  void setConnectCallback(ble_connect_cb_t cb) { connect_cb = cb; }
  void setDisconnectCallback(ble_disconnect_cb_t cb) { disconnect_cb = cb; }

  ble_connect_cb_t connect_cb = nullptr;
  ble_disconnect_cb_t disconnect_cb = nullptr;
};

class BLEAdvertisingData
{
public:
  bool addFlags(uint8_t) { return true; }
  bool addTxPower() { return true; }
  bool addName() { return true; }
  bool addService(BLEService&) { return true; }
  bool addUuid(const uint8_t*) { return true; }
};

class BLEAdvertising : public BLEAdvertisingData
{
public:
  void restartOnDisconnect(bool enable);
  void setInterval(uint16_t fast, uint16_t slow) { fast_ = fast; slow_ = slow; }
  void setFastTimeout(uint16_t sec) { fast_timeout_ = sec; }
  bool start(uint16_t timeout = 0);
  bool stop();
  bool isRunning();

  uint16_t fast_ = 32, slow_ = 244, fast_timeout_ = 30;
};

class AdafruitBluefruit
{
public:
  void configPrphBandwidth(uint8_t bw);
  bool begin(uint8_t prph_count = 1, uint8_t central_count = 0);
  void setTxPower(int8_t power);
  void setName(const char* name) { name_ = name; }
  void autoConnLed(bool enable) { auto_led = enable; }
  bool connected();

  BLEPeriph Periph;
  BLEAdvertising Advertising;
  BLEAdvertisingData ScanResponse;

  const char* name_ = "IMBA nRF52840 ProMicro";
  bool auto_led = true;
};

extern AdafruitBluefruit Bluefruit;
