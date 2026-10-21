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

// коды метрик снимка, которые нужны экрану зарядки
static const uint8_t metric_speed = 1;             // скорость, км/ч
static const uint8_t metric_battery_current = 6;   // ток АКБ, А
static const uint8_t metric_phase_current = 8;     // фазный ток, А
static const uint8_t metric_battery_temp_max = 18; // температура АКБ макс., °C
static const uint8_t metric_charge_current = 27;   // ток зарядки, А
static const uint8_t metric_charger_temp = 29;     // температура зарядника, °C
static const uint8_t metric_charging = 33;         // зарядка идёт, 0 или 1
static const int32_t metric_unknown = INT32_MIN;   // телефон эту величину не знает

// без снимка дольше этого зарядка на экране кончается: LoEUC шлёт снимок раз в секунду
static const uint32_t charge_max_age = 3000; //ms
// без признака зарядки в снимке колесо считается заряжающимся по току зарядки от этого значения
static const int32_t charge_current_min = 300; //mA
// Признак зарядки и ток зарядки шлёт только InMotion (признак ещё King Song), поэтому зарядка узнаётся и по току
// АКБ: с минусом он идёт в батарею. На ходу минус - это рекуперация, поэтому только на стоящем колесе и несколько
// снимков подряд: в конце торможения ток ещё бывает отрицательным. Закончилась зарядка, когда ток поднялся выше
// battery_charge_end: к концу зарядки ток падает, порог ниже начального, чтобы экран не мигал. И выше того, что
// берёт стоящее колесо с фарой (около 0.3 А): без знака, при стоящем моторе, это потребление похоже на зарядку
static const int32_t battery_charge_start = -1000; //mA
static const int32_t battery_charge_end = -600; //mA
static const uint8_t battery_charge_snapshots = 3;
static const int32_t charge_speed_max = 1000; //km/h * 1000
// Знак тока АКБ при зарядке у разных колёс разный. Когда мотор не работает (фазный ток меньше этого), стоящему
// колесу тратить ампер и больше не на что, и ток любого знака - это зарядка. Мотор, который держит колесо
// с райдером на склоне, берёт фазный ток в десятки ампер
static const int32_t motor_idle_current = 2000; //mA

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

// зарядка из последнего снимка и millis() его прихода
static bool charge_valid = false;
static snapshot_charge_t charge_last;
static uint32_t charge_rx_ms = 0;

static uint16_t readU16(const uint8_t* p)
{
  return ((uint16_t)p[0] << 8) | p[1];
}

static uint32_t readU32(const uint8_t* p)
{
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

// значение метрики * 1000 в десятые доли: 8250 -> 83
static int32_t milliToX10(int32_t v)
{
  return (v >= 0) ? (v + 50) / 100 : -((-v + 50) / 100);
}

// зарядка по току АКБ на стоящем колесе
static bool batteryCharging(int32_t battery_current, int32_t phase_current, int32_t speed)
{
  static uint8_t count = 0;
  // снимки подряд: после перерыва дольше charge_max_age счёт заново
  bool fresh = millis() - charge_rx_ms <= charge_max_age;
  bool was = charge_valid && fresh;
  if(!fresh) count = 0;

  bool still = speed != metric_unknown && abs(speed) < charge_speed_max;
  bool motor_idle = phase_current != metric_unknown && abs(phase_current) < motor_idle_current;
  if(battery_current != metric_unknown && motor_idle) battery_current = -abs(battery_current);
  if(battery_current == metric_unknown || !still || battery_current > battery_charge_end)
  {
    count = 0;
    return false;
  }
  if(was) return true;
  if(battery_current > battery_charge_start)
  {
    count = 0;
    return false;
  }
  if(count < battery_charge_snapshots) count++;
  return count >= battery_charge_snapshots;
}

// метрики снимка, нужные экрану зарядки. Незнакомые коды пропускаются: снимок бывает новее прошивки
static void parseCharge(const uint8_t* metrics, uint8_t count)
{
  int32_t charging = metric_unknown, charge_current = metric_unknown, battery_current = metric_unknown;
  int32_t speed = metric_unknown, phase_current = metric_unknown;
  snapshot_charge_t c = {};
  for(uint8_t i = 0; i < count; i++)
  {
    const uint8_t* m = metrics + i * metric_size;
    int32_t v = (int32_t)readU32(m + 1);
    if(v == metric_unknown) continue;
    switch(m[0])
    {
      case metric_charging: charging = v; break;
      case metric_charge_current: charge_current = v; break;
      case metric_battery_current: battery_current = v; break;
      case metric_speed: speed = v; break;
      case metric_phase_current: phase_current = v; break;
      case metric_battery_temp_max:
        c.battery_temp_x10 = constrain(milliToX10(v), -999, 9999);
        c.present |= SNAP_HAS_BATTERY_TEMP;
        break;
      case metric_charger_temp:
        c.charger_temp_x10 = constrain(milliToX10(v), -999, 9999);
        c.present |= SNAP_HAS_CHARGER_TEMP;
        break;
    }
  }

  // ток зарядки, а если колесо его не сообщает, ток АКБ: при зарядке он у разных колёс с разным знаком
  int32_t current = (charge_current != metric_unknown) ? charge_current : battery_current;
  if(current != metric_unknown)
  {
    current = abs(current);
    c.current_x10 = constrain(milliToX10(current), 0, 9999);
    c.present |= SNAP_HAS_CURRENT;
  }

  // признак зарядки, а без него ток зарядки. Признак 0 ток АКБ не перебивает: колесо, которое шлёт признак,
  // не обязательно его заполняет
  bool flagged;
  if(charging != metric_unknown) flagged = charging != 0;
  else flagged = charge_current != metric_unknown && abs(charge_current) >= charge_current_min;
  charge_valid = batteryCharging(battery_current, phase_current, speed) || flagged;
  charge_last = c;
  charge_rx_ms = millis();
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

  parseCharge(p + snap_header, p[12]);
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

bool snapshotCharge(snapshot_charge_t& charge)
{
  if(!charge_valid || millis() - charge_rx_ms > charge_max_age) return false;
  charge = charge_last;
  return true;
}

bool snapshotTimeMs(uint64_t& unix_ms)
{
  if(!time_valid) return false;
  unix_ms = time_ts + (uint32_t)(millis() - time_rx_ms);
  return true;
}
