#include "snapshot.h"
#include <Arduino.h>

// нарезка: 'L' 'E' 01, флаги (бит 0 - последний кусок), номер куска;
// у куска 0 дальше длина и CRC32 всей передачи. Числа big-endian
static const uint8_t chunk_version = 0x01;
static const uint16_t chunk_header = 6;
static const uint16_t chunk0_header = 14;

// снимок: 'L' 'E' 03, флаги, метка времени в мс (8 байт), число метрик N, записи по 5 байт
static const uint8_t snap_version = 0x03;
static const uint16_t snap_header = 13;
static const uint16_t metric_size = 5;
static const uint16_t snap_max = 512;  // 33 метрики и поездка - около 200 байт, остальное запас

// метка похожа на Unix-время в мс, если попадает в 2020-2100 годы
static const uint64_t unix_ms_min = 1577836800000ULL;
static const uint64_t unix_ms_max = 4102444800000ULL;

// сборка снимка из кусков
static uint8_t asm_buf[snap_max];
static uint16_t asm_len = 0;
static uint32_t asm_total = 0;
static uint32_t asm_crc = 0;
static uint16_t asm_next = 0;
static bool asm_active = false;

// метка времени последнего снимка и millis() его прихода
static bool time_valid = false;
static uint64_t time_ts = 0;
static uint32_t time_rx_ms = 0;

static uint16_t readU16(const uint8_t* p)
{
  return ((uint16_t)p[0] << 8) | p[1];
}

static uint32_t readU32(const uint8_t* p)
{
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static uint32_t crc32(const uint8_t* data, uint32_t len)
{
  uint32_t crc = 0xFFFFFFFF;
  while(len--)
  {
    crc ^= *data++;
    for(uint8_t i = 0; i < 8; i++) crc = (crc >> 1) ^ (0xEDB88320 & (0 - (crc & 1)));
  }
  return ~crc;
}

static void snapshotParse(const uint8_t* p, uint16_t len)
{
  if(len < snap_header || p[0] != 'L' || p[1] != 'E' || p[2] != snap_version || snap_header + p[12] * metric_size > len)
    return;

  uint64_t ts = ((uint64_t)readU32(p + 4) << 32) | readU32(p + 8);
  time_valid = ts >= unix_ms_min && ts <= unix_ms_max;
  time_ts = ts;
  time_rx_ms = millis();
}

void snapshotChunk(const uint8_t* data, uint16_t len)
{
  if(len < chunk_header || data[0] != 'L' || data[1] != 'E' || data[2] != chunk_version) return;

  bool last = data[3] & 0x01;
  uint16_t num = readU16(data + 4);
  const uint8_t* payload = data + chunk_header;
  if(num == 0)
  {
    // кусок 0 начинает новый снимок, даже если предыдущий оборвался
    asm_active = false;
    if(len < chunk0_header) return;
    asm_total = readU32(data + 6);
    asm_crc = readU32(data + 10);
    asm_len = 0;
    asm_active = true;
    payload = data + chunk0_header;
  }
  else if(!asm_active || num != asm_next)
  {
    asm_active = false;
    return;
  }

  uint16_t payload_len = len - (payload - data);
  if(asm_total > snap_max || asm_len + payload_len > asm_total)
  {
    asm_active = false;
    return;
  }
  memcpy(asm_buf + asm_len, payload, payload_len);
  asm_len += payload_len;
  asm_next = num + 1;
  if(!last) return;

  // потерянный или испорченный кусок склеился бы в мусор, похожий на данные: такой снимок не нужен
  asm_active = false;
  if(asm_len != asm_total || crc32(asm_buf, asm_len) != asm_crc) return;
  snapshotParse(asm_buf, asm_len);
}

bool snapshotTimeMs(uint64_t& unix_ms)
{
  if(!time_valid) return false;
  unix_ms = time_ts + (uint32_t)(millis() - time_rx_ms);
  return true;
}
