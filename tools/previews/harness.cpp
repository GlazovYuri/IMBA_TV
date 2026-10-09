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
  const char* names[3] = {"wave", "break", "explosion"};
  for(int anim = 0; anim < 3; anim++)
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
  return 0;
}
