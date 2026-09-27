#include "ble.h"
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
const uint32_t speed_offset = 6;
const uint32_t pwm_offset = 8;
const uint32_t charge_offset = 12;
const uint32_t limit_offset = 18;

BLEDis bledis;
BLEService euc_service(service_uuid);
BLECharacteristic euc_char(char_uuid);

static euc_data_t euc_data;

static void bleConnectCallback(uint16_t conn_hdl)
{
  euc_data.is_connected = true;
}

static void bleDisconnectCallback(uint16_t conn_hdl, uint8_t reason)
{
  euc_data.reset();
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

  tmp = bytesToInt16(data + speed_offset);
  euc_data.speed = (tmp == INT16_MIN) ? 0 : tmp / 10;

  tmp = bytesToInt16(data + pwm_offset);
  euc_data.pwm = (tmp == INT16_MIN) ? 0 : tmp / 10;

  tmp = bytesToInt16(data + charge_offset);
  euc_data.charge = (tmp == INT16_MIN) ? 0 : tmp / 10;

  tmp = bytesToInt16(data + limit_offset);
  euc_data.pwm_limit = (tmp == INT16_MIN) ? UINT8_MAX : tmp / 10;
}

static void eucCharWriteCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len)
{
  if(len != pack_len) return;
  if(!checkMagic(data)) return;

  dataParse(data);
}

void bleInit()
{
  Bluefruit.setTxPower(tx_power);

  bledis.setManufacturer("IMBA");
  bledis.setModel("IMBA TV");
  bledis.setHardwareRev("0.2");
  bledis.setFirmwareRev("0.4");
  bledis.begin();

  euc_service.begin();

  euc_char.setProperties(CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
  euc_char.setPermission(SECMODE_NO_ACCESS, SECMODE_OPEN);
  euc_char.setFixedLen(pack_len);
  euc_char.setWriteCallback(eucCharWriteCallback);
  euc_char.begin();

  Bluefruit.Periph.setConnIntervalMS(20, 30);
  Bluefruit.Periph.setConnectCallback(bleConnectCallback);
  Bluefruit.Periph.setDisconnectCallback(bleDisconnectCallback);
}

void bleAdvertise()
{
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(euc_service);
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
