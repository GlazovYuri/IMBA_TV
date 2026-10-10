#include "ble_dfu.h"

// UUID и поведение повторяют BLEDfu из Adafruit nRF52 1.7.0 (libraries/Bluefruit52Lib/src/services/BLEDfu.cpp)
static const uint16_t dfu_rev_appmode = 0x0001;

static const uint8_t uuid_svc_dfu[16] = {
  0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x30, 0x15, 0x00, 0x00,
};
static const uint8_t uuid_chr_control[16] = {
  0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x31, 0x15, 0x00, 0x00,
};
static const uint8_t uuid_chr_packet[16] = {
  0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x32, 0x15, 0x00, 0x00,
};
static const uint8_t uuid_chr_revision[16] = {
  0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x34, 0x15, 0x00, 0x00,
};

// Данные о телефоне, по которым загрузчик рекламируется с тем же адресом и телефон переподключается.
// Лежат в неинициализируемой памяти загрузчика (NOINIT) вместе с CRC
typedef struct
{
  ble_gap_addr_t    addr;
  ble_gap_irk_t     irk;
  ble_gap_enc_key_t enc_key;
  uint8_t           sys_attr[8];
} peer_data_t;

static_assert(sizeof(peer_data_t) == 60, "раскладка должна совпадать с dfu_ble_peer_data_t загрузчика");

static uint8_t* const peer_data_area = (uint8_t*) 0x20007F80UL;

// Сборки загрузчика 0.10.0 и 0.11.0 кладут в NOINIT сначала CRC (0x20007F80), а данные за ней (0x20007F82):
// такие адреса стоят в их коде. 0.6.0 ждёт данные с 0x20007F80 и CRC после них, как пишет BLEDfu
static const uint32_t crc_first_since = 0x000A00; // 0.10.0, формат bootloaderVersion: 0x00MMmmpp

extern "C" void bootloader_util_app_start(uint32_t start_addr);

static uint16_t crc16(const uint8_t* data, uint8_t length)
{
  uint16_t crc = 0xFFFF;
  while(length--)
  {
    uint8_t x = crc >> 8 ^ *data++;
    x ^= x >> 4;
    crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
  }
  return crc;
}

static void writePeerData(uint16_t conn_hdl, BLEConnection* conn)
{
  peer_data_t data;
  varclr(&data);

  uint16_t sysattr_len = sizeof(data.sys_attr);
  sd_ble_gatts_sys_attr_get(conn_hdl, data.sys_attr, &sysattr_len, BLE_GATTS_SYS_ATTR_FLAG_SYS_SRVCS);

  data.addr = conn->getPeerAddr();
  if(conn->secured())
  {
    bond_keys_t bkeys;
    if(conn->loadBondKey(&bkeys))
    {
      data.addr    = bkeys.peer_id.id_addr_info;
      data.irk     = bkeys.peer_id.id_info;
      data.enc_key = bkeys.own_enc;
    }
  }

  uint16_t crc = crc16((const uint8_t*)&data, sizeof(data));
  memset(peer_data_area, 0, sizeof(data) + sizeof(crc));
  if(bootloaderVersion >= crc_first_since)
  {
    memcpy(peer_data_area, &crc, sizeof(crc));
    memcpy(peer_data_area + sizeof(crc), &data, sizeof(data));
  }
  else
  {
    memcpy(peer_data_area, &data, sizeof(data));
    memcpy(peer_data_area + sizeof(data), &crc, sizeof(crc));
  }
}

static void controlWriteAuthorize(uint16_t conn_hdl, BLECharacteristic* chr, ble_gatts_evt_write_t* request)
{
  if(request->handle != chr->handles().value_handle ||
     request->op == BLE_GATTS_OP_PREP_WRITE_REQ ||
     request->op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW ||
     request->op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL)
  {
    return;
  }

  ble_gatts_rw_authorize_reply_params_t reply = { .type = BLE_GATTS_AUTHORIZE_TYPE_WRITE };
  if(!chr->notifyEnabled(conn_hdl))
  {
    reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_CPS_CCCD_CONFIG_ERROR;
    sd_ble_gatts_rw_authorize_reply(conn_hdl, &reply);
    return;
  }
  reply.params.write.gatt_status = BLE_GATT_STATUS_SUCCESS;
  sd_ble_gatts_rw_authorize_reply(conn_hdl, &reply);

  enum { START_DFU = 1 };
  if(request->data[0] != START_DFU) return;

  BLEConnection* conn = Bluefruit.Connection(conn_hdl);
  writePeerData(conn_hdl, conn);

  // дальше как в BLEDfu: переход в загрузчик без сброса, загрузчик возьмёт уже запущенный SoftDevice
  Bluefruit.Advertising.restartOnDisconnect(false);
  conn->disconnect();

  enum { DFU_OTA_MAGIC = 0xB1 };
  sd_power_gpregret_clr(0, 0xFF);
  if(sd_power_gpregret_set(0, DFU_OTA_MAGIC) != NRF_SUCCESS) return;
  if(sd_softdevice_disable() != NRF_SUCCESS) return;

  NVIC->ICER[0] = 0xFFFFFFFF;
  NVIC->ICPR[0] = 0xFFFFFFFF;
#if defined(__NRF_NVIC_ISER_COUNT) && __NRF_NVIC_ISER_COUNT == 2
  NVIC->ICER[1] = 0xFFFFFFFF;
  NVIC->ICPR[1] = 0xFFFFFFFF;
#endif

  if(sd_softdevice_vector_table_base_set(NRF_UICR->NRFFW[0]) != NRF_SUCCESS) return;
  __set_CONTROL(0); // стек MSP, как требует переход из-под FreeRTOS
  bootloader_util_app_start(NRF_UICR->NRFFW[0]);
}

BleDfuService::BleDfuService()
  : BLEService(uuid_svc_dfu), _chr_control(uuid_chr_control)
{
}

err_t BleDfuService::begin()
{
  VERIFY_STATUS(BLEService::begin());

  BLECharacteristic chr_packet(uuid_chr_packet);
  chr_packet.setTempMemory();
  chr_packet.setProperties(CHR_PROPS_WRITE_WO_RESP);
  chr_packet.setMaxLen(20);
  VERIFY_STATUS(chr_packet.begin());

  _chr_control.setProperties(CHR_PROPS_WRITE | CHR_PROPS_NOTIFY);
  _chr_control.setMaxLen(23);
  _chr_control.setWriteAuthorizeCallback(controlWriteAuthorize);
  VERIFY_STATUS(_chr_control.begin());

  BLECharacteristic chr_revision(uuid_chr_revision);
  chr_revision.setTempMemory();
  chr_revision.setProperties(CHR_PROPS_READ);
  chr_revision.setFixedLen(2);
  VERIFY_STATUS(chr_revision.begin());
  chr_revision.write16(dfu_rev_appmode);

  return ERROR_NONE;
}
