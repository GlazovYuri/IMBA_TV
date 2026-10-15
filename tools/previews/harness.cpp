// Стенд превью анимаций: настоящий код экранов и анимаций прошивки, собранный на компьютере.
// Wire ведёт в эмулятор контроллера SSH1106, время виртуальное: идёт в delay() и при передаче
// байтов по I2C со скоростью 400 кГц, как на плате. Каждый показанный кадр пишется в файл.
// Запуск через tools/render_previews.py.
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <vector>
#include <string>
#include "animations.h"
#include "config.h"
#include "display.h"
#include "screen_charge.h"
#include "snapshot.h"
#include "utils.h"

// ===== Время =====

static uint64_t now_us = 0;
uint32_t millis() { return (uint32_t)(now_us / 1000); }
uint32_t micros() { return (uint32_t)now_us; }
void delay(uint32_t ms) { now_us += (uint64_t)ms * 1000; }
void delayMicroseconds(uint32_t us) { now_us += us; }

// ===== Заглушки платы =====

SPIClass SPI;
const char* bootloaderVersionText() { return "0.10.0"; }
bool buttonPressed() { return false; }

// ===== Эмулятор SSH1106 =====

struct frame_t
{
  uint64_t t_us;
  uint8_t contrast;
  bool inverted;
  uint8_t pixels[128 * 64 / 8];  // строка за строкой, 1 бит на точку, старший бит слева
};

static std::vector<frame_t> frames;
static uint8_t ram[8][132];
static int page = 0, col = 0;
static uint8_t contrast = 0xFF;
static bool inverted = false;

static void snapshot()
{
  frame_t f;
  f.t_us = now_us;
  f.contrast = contrast;
  f.inverted = inverted;
  memset(f.pixels, 0, sizeof(f.pixels));
  // на модуле 128x64 видны столбцы 2..129 памяти SSH1106
  for(int y = 0; y < 64; y++)
    for(int x = 0; x < 128; x++)
      if(ram[y >> 3][x + 2] >> (y & 7) & 1) f.pixels[(y * 128 + x) >> 3] |= 0x80 >> (x & 7);
  frames.push_back(f);
}

static void command(const uint8_t* cmd, size_t n)
{
  for(size_t i = 0; i < n; i++)
  {
    uint8_t c = cmd[i];
    if(c >= 0xB0 && c <= 0xB7) page = c - 0xB0;
    else if(c <= 0x0F) col = (col & 0xF0) | c;
    else if(c >= 0x10 && c <= 0x1F) col = (col & 0x0F) | ((c & 0x0F) << 4);
    else if(c == 0x81 && i + 1 < n) { contrast = cmd[++i]; snapshot(); }
    else if(c == 0xA6 || c == 0xA7) { inverted = (c == 0xA7); snapshot(); }
    else if(c == 0x20 || c == 0x8D || c == 0xA8 || c == 0xD3 || c == 0xD5 || c == 0xD9 || c == 0xDA || c == 0xDB || c == 0xAD) i++;
    else if(c == 0x21 || c == 0x22) i += 2;
  }
}

static void data(const uint8_t* bytes, size_t n)
{
  for(size_t i = 0; i < n; i++)
  {
    if(col < 132) ram[page][col] = bytes[i];
    col++;
    // кадр дописан, когда заполнена последняя видимая колонка последней страницы
    if(page == 7 && col == 130) snapshot();
  }
}

TwoWire Wire;
static std::vector<uint8_t> tx;

void TwoWire::beginTransmission(uint8_t) { tx.clear(); }
size_t TwoWire::write(uint8_t b) { tx.push_back(b); return 1; }

uint8_t TwoWire::endTransmission(bool)
{
  // адрес и байты по 9 бит на 400 кГц, плюс старт и стоп
  now_us += (tx.size() + 1) * 9 * 1000000ULL / 400000 + 5;
  if(tx.empty()) return 0;
  if(tx[0] == 0x40) data(tx.data() + 1, tx.size() - 1);
  else command(tx.data() + 1, tx.size() - 1);
  return 0;
}

// ===== Сценарии =====

static void save(const char* path)
{
  FILE* f = fopen(path, "wb");
  if(!f) { perror(path); exit(1); }
  for(const frame_t& fr : frames)
  {
    uint32_t t_ms = (uint32_t)(fr.t_us / 1000);
    fwrite(&t_ms, 4, 1, f);
    fputc(fr.contrast, f);
    fputc(fr.inverted, f);
    fwrite(fr.pixels, sizeof(fr.pixels), 1, f);
  }
  // конец записи: время последнего кадра нужно, чтобы знать, сколько он висит
  uint32_t end_ms = (uint32_t)(now_us / 1000);
  fwrite(&end_ms, 4, 1, f);
  fclose(f);
  frames.clear();
}

static euc_data_t sampleData()
{
  euc_data_t d;
  d.is_connected = true;
  d.speed = 27;
  d.pwm = 41;
  d.charge = 76;
  d.pwm_limit = 80;
  d.voltage_x10 = 1418;
  d.temperature_x10 = 384;
  d.trip_x10 = 124;
  d.present = EUC_HAS_SPEED | EUC_HAS_PWM | EUC_HAS_VOLTAGE | EUC_HAS_CHARGE | EUC_HAS_TEMPERATURE | EUC_HAS_TRIP;
  d.frame_interval_ms = 120;
  d.frame_last_interval_ms = 95;
  d.last_frame_ms = 0;
  return d;
}

static const uint8_t dev_charge = 82;
static const uint32_t hold_iface = 1600;  // интерфейс после перехода, мс

// подготовка экрана в запись не попадает: гифка начинается с первого кадра сценария
static void begin()
{
  displaySetBrightness(100);
  displayClear();
  frames.clear();
}

// ===== Экраны для редактора на сайте =====
// Сайт рисует экраны своим кодом (web/js/screens.js). Здесь те же экраны снимает настоящий код прошивки,
// тест сайта сверяет с ними свою отрисовку. Запись: параметры экрана и данные, затем 1024 байта памяти
// дисплея (страница за страницей, столбцы 0..127)

static fw_config_t& configMut() { return const_cast<fw_config_t&>(config()); }

static euc_data_t wideData()
{
  euc_data_t d = sampleData();
  d.speed = 123;
  d.pwm = 100;
  d.charge = 100;
  d.voltage_x10 = 1512;
  d.temperature_x10 = -153;
  d.trip_x10 = 9999;
  return d;
}

static void writeScreen(FILE* f, uint8_t type, const uint8_t v[4], uint16_t wheel, euc_data_t d, uint8_t dev, bool charging,
                        uint8_t timezone = CONFIG_TIMEZONE_UTC_P3)
{
  fw_config_t& cfg = configMut();
  cfg.screen1_type = type;
  cfg.screen1_v1 = v[0];
  cfg.screen1_v2 = v[1];
  cfg.screen1_v3 = v[2];
  cfg.screen1_v4 = v[3];
  cfg.screen2_type = cfg.screen3_type = cfg.screen4_type = cfg.screen5_type = 0;
  cfg.wheel_voltage_x10 = wheel;
  cfg.timezone = timezone;
  displayInit();
  d.last_frame_ms = millis() - 10;  // кадр от телефона только что пришёл
  // время, которое увидят часы: до отрисовки шина не занята, millis() не сдвинется
  uint64_t unix_ms = 0;
  bool has_time = snapshotTimeMs(unix_ms);
  displayDrawIface(dev, charging, d);
  frames.clear();

  uint8_t head[48];
  int n = 0;
  head[n++] = type;
  for(int i = 0; i < 4; i++) head[n++] = v[i];
  head[n++] = wheel & 0xFF; head[n++] = wheel >> 8;
  head[n++] = dev;
  head[n++] = charging;
  head[n++] = d.is_connected;
  head[n++] = d.present;
  head[n++] = d.speed & 0xFF; head[n++] = d.speed >> 8;
  head[n++] = d.pwm;
  head[n++] = d.charge;
  head[n++] = d.voltage_x10 & 0xFF; head[n++] = d.voltage_x10 >> 8;
  head[n++] = (uint16_t)d.temperature_x10 & 0xFF; head[n++] = (uint16_t)d.temperature_x10 >> 8;
  head[n++] = d.trip_x10 & 0xFF; head[n++] = d.trip_x10 >> 8;
  head[n++] = d.frame_interval_ms & 0xFF; head[n++] = d.frame_interval_ms >> 8;
  head[n++] = d.frame_last_interval_ms & 0xFF; head[n++] = d.frame_last_interval_ms >> 8;
  head[n++] = timezone;
  head[n++] = has_time;
  for(int i = 0; i < 8; i++) head[n++] = unix_ms >> (i * 8);
  fwrite(head, 1, n, f);
  for(int page = 0; page < 8; page++) fwrite(&ram[page][2], 1, 128, f);
}

static uint32_t crc32(const uint8_t* data, size_t len)
{
  uint32_t crc = 0xFFFFFFFF;
  while(len--)
  {
    crc ^= *data++;
    for(int i = 0; i < 8; i++) crc = (crc >> 1) ^ (0xEDB88320 & (0 - (crc & 1)));
  }
  return ~crc;
}

// метрика снимка: код и значение * 1000
struct metric_t
{
  uint8_t code;
  int32_t value;
};

// снимок LoEUC с меткой времени ts одним куском, как его пишет телефон при большом MTU
static void feedSnapshot(uint64_t ts, const metric_t* metrics = nullptr, uint8_t count = 0)
{
  uint8_t snap[13 + 5 * 33] = {'L', 'E', 3, 0};
  for(int i = 0; i < 8; i++) snap[4 + i] = ts >> (56 - i * 8);
  snap[12] = count;
  for(int i = 0; i < count; i++)
  {
    uint8_t* m = snap + 13 + i * 5;
    m[0] = metrics[i].code;
    for(int b = 0; b < 4; b++) m[1 + b] = (uint32_t)metrics[i].value >> (24 - b * 8);
  }
  uint16_t len = 13 + 5 * count;
  uint32_t crc = crc32(snap, len);
  uint8_t chunk[14 + sizeof(snap)] = {'L', 'E', 1, 1, 0, 0, 0, 0, (uint8_t)(len >> 8), (uint8_t)len,
                                      (uint8_t)(crc >> 24), (uint8_t)(crc >> 16), (uint8_t)(crc >> 8), (uint8_t)crc};
  memcpy(chunk + 14, snap, len);
  snapshotChunk(chunk, 14 + len);
}

// колесо на зарядке: идёт зарядка, ток 8.2 А, самая горячая точка батареи 34.6 °C, зарядник 41.3 °C
static const metric_t charging_metrics[] = {{33, 1000}, {27, 8200}, {18, 34600}, {29, 41300}};

static void saveScreens(const char* path)
{
  FILE* f = fopen(path, "wb");
  if(!f) { perror(path); exit(1); }
  const uint16_t wheels[3] = {840, 1008, 1512};
  euc_data_t sample = sampleData(), wide = wideData(), none = sampleData(), trip = sampleData();
  none.present = 0;
  trip.trip_x10 = 1234;
  trip.speed = 103;

  // два значения: все сочетания, у каждого обычные и самые широкие данные
  const uint8_t big[3] = {0, 1, 4};
  for(int top = 2; top <= 3; top++)
    for(int l = 0; l < 3; l++)
      for(int r = 0; r < 3; r++)
      {
        uint8_t v[4] = {(uint8_t)top, big[l], big[r], 5};
        writeScreen(f, 1, v, 1512, sample, 82, false);
        writeScreen(f, 1, v, wheels[(l + r) % 3], wide, 100, (l + r) % 2);
      }
  const uint8_t main_default[4] = {2, 0, 1, 5};
  writeScreen(f, 1, main_default, 1512, none, 5, false);
  // часы сверху: снимка ещё не было, на часах прочерки
  const uint8_t main_clock[4] = {6, 0, 1, 5};
  writeScreen(f, 1, main_clock, 1512, sample, 82, false);
  writeScreen(f, 1, main_clock, 1512, none, 5, true);

  // сетка: все пары в верхней строке при разных нижних, так проходят все варианты разметки
  uint32_t rnd = 0x1234567u;
  for(int a = 0; a < 6; a++)
    for(int b = 0; b < 6; b++)
    {
      rnd = rnd * 1103515245u + 12345u;
      uint8_t v[4] = {(uint8_t)a, (uint8_t)b, (uint8_t)((rnd >> 16) % 6), (uint8_t)((rnd >> 20) % 6)};
      euc_data_t d = (a + b) % 3 == 0 ? wide : ((a + b) % 3 == 1 ? sample : trip);
      writeScreen(f, 2, v, wheels[(a * 6 + b) % 3], d, 82, false);
    }
  const uint8_t grid_default[4] = {2, 3, 4, 5};
  writeScreen(f, 2, grid_default, 1512, none, 82, false);

  // связь и версии
  writeScreen(f, 3, grid_default, 1512, sample, 82, false);
  euc_data_t slow = sampleData();
  slow.frame_interval_ms = 2400;
  slow.frame_last_interval_ms = 1840;
  writeScreen(f, 3, grid_default, 1512, slow, 82, false);
  euc_data_t off = sampleData();
  off.is_connected = false;
  writeScreen(f, 3, grid_default, 1512, off, 82, false);

  // часы: без снимков времени нет, потом метки с разными поясами - смена суток, месяца, года и все дни недели
  writeScreen(f, 4, grid_default, 1512, sample, 82, false);
  const struct { uint64_t ts; uint8_t timezone; } clocks[] = {
    {1791806430000ULL, CONFIG_TIMEZONE_UTC_P3},   // пн 12.10.2026 15:00
    {1791892830000ULL, CONFIG_TIMEZONE_UTC_P3},   // вт 13.10 15:00
    {1791979230000ULL, CONFIG_TIMEZONE_UTC_P5},   // ср 14.10 17:00
    {1792065630000ULL, CONFIG_TIMEZONE_UTC_0},    // чт 15.10 12:00
    {1792152030000ULL, CONFIG_TIMEZONE_UTC_M5},   // пт 16.10 07:00
    {1792238430000ULL, CONFIG_TIMEZONE_UTC_P3},   // сб 17.10 15:00
    {1792238430000ULL, CONFIG_TIMEZONE_UTC_P12},  // та же метка на Камчатке: вс 18.10 00:00
    {1792324830000ULL, CONFIG_TIMEZONE_UTC_P14},  // вс 18.10 12:00 UTC, в UTC+14 уже пн 19.10 02:00
    {1798756200000ULL, CONFIG_TIMEZONE_UTC_P3},   // 31.12.2026 22:30 UTC, в Москве уже пт 01.01.2027 01:30
    {1803870900000ULL, CONFIG_TIMEZONE_UTC_M12},  // 01.03.2027 03:15 UTC, в UTC-12 ещё вс 28.02 15:15
    {1835999999000ULL, CONFIG_TIMEZONE_UTC_0},    // пн 06.03.2028 23:59:59
    {1835438400000ULL, CONFIG_TIMEZONE_UTC_P10},  // високосный год: вт 29.02.2028 22:00
  };

  for(const auto& c : clocks)
  {
    feedSnapshot(c.ts);
    writeScreen(f, 4, grid_default, 1512, sample, 82, false, c.timezone);
  }
  // часы сверху на экране с двумя значениями: время последнего снимка, заряд колеса от 0 до 100 %
  euc_data_t low = sampleData();
  low.charge = 5;
  euc_data_t empty = sampleData();
  empty.charge = 0;
  writeScreen(f, 1, main_clock, 1512, sample, 82, false, CONFIG_TIMEZONE_UTC_P3);
  writeScreen(f, 1, main_clock, 1512, wide, 100, true, CONFIG_TIMEZONE_UTC_M5);
  writeScreen(f, 1, main_clock, 1008, low, 9, false, CONFIG_TIMEZONE_UTC_P14);
  writeScreen(f, 1, main_clock, 840, empty, 100, false, CONFIG_TIMEZONE_UTC_0);
  // время идёт и без связи
  writeScreen(f, 4, grid_default, 1512, off, 82, false);
  // метка не похожа на Unix-время: часов нет
  feedSnapshot(123456789ULL);
  writeScreen(f, 4, grid_default, 1512, sample, 82, false);
  fclose(f);
}

int main(int argc, char** argv)
{
  std::string out = argc > 1 ? argv[1] : ".";
  uint32_t seed = argc > 2 ? strtoul(argv[2], nullptr, 0) : 0x5EED1234u;

  configLoad();
  displayInit();
  euc_data_t d = sampleData();

  // логотип секунду, затем сразу данные
  begin();
  displayDrawLogo();
  delay(1000);
  displayDrawIface(dev_charge, false, d);
  delay(hold_iface);
  save((out + "/logo.bin").c_str());

  // заставки: логотип секунду и переход, как displayIntroBegin + displayIntroFinish
  const char* names[6] = {"wave", "break", "explosion", "clawd_type", "clawd_laptop", "clawd_pixels"};
  for(int anim = 0; anim < 6; anim++)
  {
    begin();
    displayPlayIntroNum(anim, seed, dev_charge, false, d);
    displayDrawIface(dev_charge, false, d);
    delay(hold_iface);
    save((out + "/" + names[anim] + ".bin").c_str());
  }

  // выключение: данные, затем схлопывание экрана
  begin();
  displayDrawIface(dev_charge, false, d);
  delay(1200);
  displayPowerOffAnimation(dev_charge, false, d);
  delay(900);
  save((out + "/poweroff.bin").c_str());

  saveScreens((out + "/screens.bin").c_str());

  // зарядка: снимок раз в секунду, экран зарядки открывается сам. Оформление при открытии случайное,
  // здесь все по очереди, каждое со своим появлением
  const uint32_t charge_hold[charge_styles] = {4200, 5000, 4400};
  fw_config_t& cfg = configMut();
  cfg = FW_CONFIG_DEFAULTS;
  begin();
  for(uint8_t style = 0; style < charge_styles; style++)
  {
    displayInit();
    displaySetBrightness(100);
    feedSnapshot(1791806430000ULL, charging_metrics, sizeof(charging_metrics) / sizeof(charging_metrics[0]));
    d.last_frame_ms = millis();
    displayRenderIface(dev_charge, false, d);  // открывает экран зарядки, на дисплей кадр не уходит
    screenChargeOpen(style);
    uint32_t start = millis(), next_snap = start + 1000;
    while(millis() - start < charge_hold[style])
    {
      if((int32_t)(millis() - next_snap) >= 0)
      {
        feedSnapshot(1791806430000ULL, charging_metrics, sizeof(charging_metrics) / sizeof(charging_metrics[0]));
        next_snap += 1000;
      }
      d.last_frame_ms = millis();
      displayDrawIface(dev_charge, false, d);
      delay(40);
    }
  }
  save((out + "/charging.bin").c_str());
  return 0;
}
