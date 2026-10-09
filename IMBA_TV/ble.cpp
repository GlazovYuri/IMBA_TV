#include "ble.h"
#include "version.h"
#include <bluefruit.h>

const int8_t tx_power = 8; //dBm

const uint8_t service_uuid[] = {
  0x01, 0x2c, 0x7c, 0xd9, 0x4f, 0x2a, 0x61, 0xae,
  0x96, 0x4d, 0x1a, 0x6d, 0x9c, 0x2f, 0x0f, 0x1d,
};

const uint8_t char_uuid[] = {
  0x02, 0x2c, 0x7c, 0xd9, 0x4f, 0x2a, 0x61, 0xae,
  0x96, 0x4d, 0x1a, 0x6d, 0x9c, 0x2f, 0x0f, 0x1d,
};

const uint16_t pack_len = 20;
// формат пакета: https://loeuc.ru/ru/docs/devices
const uint32_t speed_offset = 6;
const uint32_t pwm_offset = 8;
const uint32_t voltage_offset = 10;
const uint32_t charge_offset = 12;
const uint32_t temperature_offset = 14;
const uint32_t trip_offset = 16;
const uint32_t limit_offset = 18;

BLEDfu bledfu;
BLEDis bledis;
BLEService euc_service(service_uuid);
BLECharacteristic euc_char(char_uuid);

static euc_data_t euc_data;

// среднее по последним frame_hist интервалам между кадрами
static const uint8_t frame_hist = 10;
static uint32_t frame_intervals[frame_hist];
static uint8_t frame_hist_cnt = 0;
static uint8_t frame_hist_pos = 0;

static void frameTimingReset()
{
  frame_hist_cnt = 0;
  frame_hist_pos = 0;
  euc_data.frame_interval_ms = 0;
  euc_data.last_frame_ms = 0;
}

static void frameTimingUpdate()
{
  uint32_t now = millis();
  if(euc_data.last_frame_ms != 0)
  {
    frame_intervals[frame_hist_pos] = now - euc_data.last_frame_ms;
    frame_hist_pos = (frame_hist_pos + 1) % frame_hist;
    if(frame_hist_cnt < frame_hist) frame_hist_cnt++;

    uint32_t sum = 0;
    for(uint8_t i = 0; i < frame_hist_cnt; i++) sum += frame_intervals[i];
    uint32_t avg = sum / frame_hist_cnt;
    euc_data.frame_interval_ms = (avg > UINT16_MAX) ? UINT16_MAX : (avg == 0 ? 1 : avg);
  }
  euc_data.last_frame_ms = (now == 0) ? 1 : now;
}

static void bleConnectCallback(uint16_t conn_hdl)
{
  frameTimingReset();
  euc_data.is_connected = true;
}

static void bleDisconnectCallback(uint16_t conn_hdl, uint8_t reason)
{
  euc_data.reset();
  frameTimingReset();
}

static bool checkMagic(uint8_t* data)
{
  return data[0] == 'L' && data[1] == 'E' && data[2] == 0x1;
}

static int16_t bytesToInt16(uint8_t* bytes)
{
  return ((uint16_t)bytes[0] << 8) + bytes[1];
}

static void dataParse(uint8_t* data)
{
  euc_data.frame_cnt = data[3];

  int16_t tmp;
  uint8_t present = 0;

  tmp = bytesToInt16(data + speed_offset);
  euc_data.speed = (tmp == INT16_MIN) ? 0 : tmp / 10;
  if(tmp != INT16_MIN) present |= EUC_HAS_SPEED;

  tmp = bytesToInt16(data + pwm_offset);
  euc_data.pwm = (tmp == INT16_MIN) ? 0 : tmp / 10;
  if(tmp != INT16_MIN) present |= EUC_HAS_PWM;

  tmp = bytesToInt16(data + voltage_offset);
  euc_data.voltage_x10 = (tmp == INT16_MIN || tmp < 0) ? 0 : tmp;
  if(tmp != INT16_MIN) present |= EUC_HAS_VOLTAGE;

  tmp = bytesToInt16(data + charge_offset);
  euc_data.charge = (tmp == INT16_MIN) ? 0 : tmp / 10;
  if(tmp != INT16_MIN) present |= EUC_HAS_CHARGE;

  tmp = bytesToInt16(data + temperature_offset);
  euc_data.temperature_x10 = (tmp == INT16_MIN) ? 0 : tmp;
  if(tmp != INT16_MIN) present |= EUC_HAS_TEMPERATURE;

  // пробег не бывает отрицательным: читаем как беззнаковое, это даёт до 6553.4 км вместо 3276.7
  tmp = bytesToInt16(data + trip_offset);
  euc_data.trip_x10 = (tmp == INT16_MIN) ? 0 : (uint16_t)tmp;
  if(tmp != INT16_MIN) present |= EUC_HAS_TRIP;

  tmp = bytesToInt16(data + limit_offset);
  euc_data.pwm_limit = (tmp == INT16_MIN) ? UINT8_MAX : tmp / 10;
  if(tmp != INT16_MIN) present |= EUC_HAS_PWM_LIMIT;

  euc_data.present = present;
}

static void eucCharWriteCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len)
{
  if(len != pack_len) return;
  if(!checkMagic(data)) return;

  dataParse(data);
  frameTimingUpdate();
}

void bleInit(bool update_mode)
{
  Bluefruit.setTxPower(tx_power);

  // сервис обновления первым, как в примерах Adafruit
  if(update_mode) bledfu.begin();

  bledis.setManufacturer("IMBA");
  bledis.setModel("IMBA TV");
  bledis.setHardwareRev("0.2");
  bledis.setFirmwareRev(FW_VERSION);
  bledis.begin();

  if(!update_mode)
  {
    euc_service.begin();

    euc_char.setProperties(CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
    euc_char.setPermission(SECMODE_NO_ACCESS, SECMODE_OPEN);
    euc_char.setFixedLen(pack_len);
    euc_char.setWriteCallback(eucCharWriteCallback);
    euc_char.begin();
  }

  Bluefruit.Periph.setConnIntervalMS(20, 30);
  Bluefruit.Periph.setConnectCallback(bleConnectCallback);
  Bluefruit.Periph.setDisconnectCallback(bleDisconnectCallback);
}

void bleAdvertise(bool update_mode)
{
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  // в режиме обновления сайт ищет плату по сервису DFU, а LoEUC её не видит
  if(update_mode) Bluefruit.Advertising.addService(bledfu);
  else Bluefruit.Advertising.addService(euc_service);
  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);

  Bluefruit.Advertising.start(0);
}

euc_data_t& bleGetEucData()
{
  return euc_data;
}
