#include "screen_snapshot.h"
#include "GyverOLED.h"

extern GyverOLED<SSH1106_128x64> oled;

static const int scr_width = 128;
static const int row_height = 8;

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

// уход часов: (millis() - метка) от начала замера, минимум за первую минуту и за последние одну-две.
// Минимум отсекает задержки радио, разница минимумов - насколько часы дисплея убежали от телефона
// за время между снимками, на которых эти минимумы пришлись
static const uint32_t drift_window_ms = 60000;
// ppm показывается после 10 минут замера: раньше разброс задержек больше самого ухода
static const uint32_t drift_ppm_min_ms = 600000;
// метка ушла назад или шагнула иначе, чем millis(), больше чем на 5 с: время на телефоне перевели, замер заново
static const int64_t drift_jump_ms = 5000;

// сборка снимка из кусков
static uint8_t asm_buf[snap_max];
static uint16_t asm_len = 0;
static uint32_t asm_total = 0;
static uint32_t asm_crc = 0;
static uint16_t asm_next = 0;
static uint8_t asm_chunks = 0;
static bool asm_active = false;

struct snapshot_stat_t
{
  uint32_t count;       // собранных снимков
  uint32_t errors;      // кривых и недошедших
  uint32_t crc_errors;  // CRC не совпал: снимок всё равно показывается, чтобы было видно, что пришло
  uint64_t ts;          // метка времени последнего снимка
  uint32_t rx_ms;       // millis() его прихода
  int32_t ts_step;      // шаг метки между двумя последними снимками
  uint32_t rx_step;     // шаг millis() между ними
  uint8_t metrics;
  uint16_t len;
  uint8_t chunks;

  bool drift_on;
  uint64_t base_ts;
  uint32_t base_ms;
  // минимумы и метки их снимков, мс от начала замера
  int32_t base_min;
  uint32_t base_min_t;
  int32_t win_min;
  uint32_t win_min_t;
  int32_t win_prev_min;
  uint32_t win_prev_min_t;
  bool win_prev_valid;
  uint32_t win_start_ms;
};

static snapshot_stat_t snap;

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

static void driftUpdate(uint64_t ts, uint32_t now)
{
  bool restart = !snap.drift_on;
  if(snap.drift_on)
  {
    int64_t gap = (int64_t)(uint32_t)(now - snap.rx_ms) - (int64_t)(ts - snap.ts);
    if(ts < snap.ts || gap > drift_jump_ms || gap < -drift_jump_ms) restart = true;
  }
  if(restart)
  {
    snap.drift_on = true;
    snap.base_ts = ts;
    snap.base_ms = now;
    snap.base_min = 0;
    snap.base_min_t = 0;
    snap.win_min = 0;
    snap.win_min_t = 0;
    snap.win_prev_valid = false;
    snap.win_start_ms = now;
    return;
  }

  uint32_t t = ts - snap.base_ts;
  int32_t d = (int32_t)((int64_t)(uint32_t)(now - snap.base_ms) - (int64_t)t);
  if(t < drift_window_ms && d < snap.base_min)
  {
    snap.base_min = d;
    snap.base_min_t = t;
  }
  if(now - snap.win_start_ms >= drift_window_ms)
  {
    snap.win_prev_min = snap.win_min;
    snap.win_prev_min_t = snap.win_min_t;
    snap.win_prev_valid = true;
    snap.win_min = d;
    snap.win_min_t = t;
    snap.win_start_ms = now;
  }
  else if(d < snap.win_min)
  {
    snap.win_min = d;
    snap.win_min_t = t;
  }
}

static void snapshotParse(const uint8_t* p, uint16_t len, uint8_t chunks)
{
  if(len < snap_header || p[0] != 'L' || p[1] != 'E' || p[2] != snap_version || snap_header + p[12] * metric_size > len)
  {
    snap.errors++;
    return;
  }

  uint64_t ts = ((uint64_t)readU32(p + 4) << 32) | readU32(p + 8);
  uint32_t now = millis();
  if(snap.count > 0)
  {
    snap.ts_step = (int32_t)(int64_t)(ts - snap.ts);
    snap.rx_step = now - snap.rx_ms;
  }
  driftUpdate(ts, now);

  snap.ts = ts;
  snap.rx_ms = now;
  snap.metrics = p[12];
  snap.len = len;
  snap.chunks = chunks;
  snap.count++;
}

void snapshotChunk(const uint8_t* data, uint16_t len)
{
  if(len < chunk_header || data[0] != 'L' || data[1] != 'E' || data[2] != chunk_version)
  {
    snap.errors++;
    return;
  }

  bool last = data[3] & 0x01;
  uint16_t num = readU16(data + 4);
  const uint8_t* payload = data + chunk_header;
  if(num == 0)
  {
    if(asm_active) snap.errors++;  // предыдущий снимок оборвался
    asm_active = false;
    if(len < chunk0_header)
    {
      snap.errors++;
      return;
    }
    asm_total = readU32(data + 6);
    asm_crc = readU32(data + 10);
    asm_len = 0;
    asm_chunks = 0;
    asm_active = true;
    payload = data + chunk0_header;
  }
  else if(!asm_active || num != asm_next)
  {
    snap.errors++;
    asm_active = false;
    return;
  }

  uint16_t payload_len = len - (payload - data);
  if(asm_total > snap_max || asm_len + payload_len > asm_total)
  {
    snap.errors++;
    asm_active = false;
    return;
  }
  memcpy(asm_buf + asm_len, payload, payload_len);
  asm_len += payload_len;
  asm_chunks++;
  asm_next = num + 1;
  if(!last) return;

  asm_active = false;
  if(asm_len != asm_total)
  {
    snap.errors++;
    return;
  }
  if(crc32(asm_buf, asm_len) != asm_crc) snap.crc_errors++;
  snapshotParse(asm_buf, asm_len, asm_chunks);
}

// printf из newlib-nano не умеет 64-битные числа
static void formatU64(char* out, uint64_t v)
{
  char tmp[21];
  int n = 0;
  do
  {
    tmp[n++] = '0' + v % 10;
    v /= 10;
  } while(v);
  for(int i = 0; i < n; i++) out[i] = tmp[n - 1 - i];
  out[n] = 0;
}

// дата и время UTC из Unix-времени в мс (алгоритм civil_from_days Говарда Хиннанта)
static void formatUtc(char* out, size_t size, uint64_t ms)
{
  uint32_t secs = (ms / 1000) % 86400;
  uint32_t days = ms / 86400000 + 719468;
  uint32_t era = days / 146097;
  uint32_t doe = days - era * 146097;
  uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  uint32_t mp = (5 * doy + 2) / 153;
  uint32_t day = doy - (153 * mp + 2) / 5 + 1;
  uint32_t month = mp < 10 ? mp + 3 : mp - 9;
  uint32_t year = yoe + era * 400 + (month <= 2);
  snprintf(out, size, "%02u.%02u.%02u %02u:%02u:%02u UTC", (unsigned)day, (unsigned)month, (unsigned)(year % 100),
           (unsigned)(secs / 3600), (unsigned)(secs / 60 % 60), (unsigned)(secs % 60));
}

// подпись у левого края, значение у правого
static void drawRow(int row, const char* label, const char* value)
{
  int y = row * row_height;
  oled.setCursorXY(0, y);
  oled.print(label);

  // знак 6 px; в UTF-8 считаем только первые байты символов
  int chars = 0;
  for(const char* p = value; *p; p++) if((*p & 0xC0) != 0x80) chars++;
  oled.setCursorXY(scr_width - chars * 6, y);
  oled.print(value);
}

void screenSnapshotDraw()
{
  char label[24];
  char text[32];
  bool have = snap.count > 0;

  oled.setScale(1);
  snprintf(text, sizeof(text), "%lu", (unsigned long)snap.count);
  drawRow(0, "Снимок 2c07", text);

  if(have) formatU64(text, snap.ts);
  drawRow(1, "ts", have ? text : "--");

  // время идёт от последней метки по millis(), как шли бы часы после синхронизации
  oled.setCursorXY(0, 2 * row_height);
  if(!have) oled.print("--");
  else if(snap.ts < unix_ms_min || snap.ts > unix_ms_max) oled.print("ts не Unix-время");
  else
  {
    formatUtc(text, sizeof(text), snap.ts + (uint32_t)(millis() - snap.rx_ms));
    oled.print(text);
  }

  if(snap.count > 1) snprintf(text, sizeof(text), "%ld/%lu", (long)snap.ts_step, (unsigned long)snap.rx_step);
  drawRow(3, "шаг ts/мс", snap.count > 1 ? text : "--");

  if(have) snprintf(text, sizeof(text), "%u/%u/%u", snap.metrics, snap.len, snap.chunks);
  drawRow(4, "мет/байт/кус", have ? text : "--");

  snprintf(text, sizeof(text), "%lu, %lu", (unsigned long)snap.errors, (unsigned long)snap.crc_errors);
  drawRow(5, "ошибки, CRC", text);

  // уход: плюс - часы дисплея спешат относительно телефона
  bool prev = snap.win_prev_valid && snap.win_prev_min < snap.win_min;
  int32_t drift = (prev ? snap.win_prev_min : snap.win_min) - snap.base_min;
  uint32_t span = (prev ? snap.win_prev_min_t : snap.win_min_t) - snap.base_min_t;
  if(!snap.drift_on || span < drift_window_ms)
  {
    drawRow(6, "уход", "--");
    drawRow(7, "уход, ppm", "--");
    return;
  }
  snprintf(label, sizeof(label), "уход, %lu мин", (unsigned long)(span / 60000));
  snprintf(text, sizeof(text), "%c%ld мс", drift < 0 ? '-' : '+', labs(drift));
  drawRow(6, label, text);

  if(span < drift_ppm_min_ms)
  {
    drawRow(7, "уход, ppm", "--");
    return;
  }
  int64_t ppm_x10 = (int64_t)drift * 10000000 / span;
  uint32_t ppm_abs = ppm_x10 < 0 ? -ppm_x10 : ppm_x10;
  snprintf(text, sizeof(text), "%c%lu.%lu", ppm_x10 < 0 ? '-' : '+', (unsigned long)(ppm_abs / 10),
           (unsigned long)(ppm_abs % 10));
  drawRow(7, "уход, ppm", text);
}
