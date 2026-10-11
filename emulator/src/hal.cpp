// Реализация "железа" для прошивки. Этот код выполняется в процессе прошивки
// и пишет состояние платы в общую память (shared.h).
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <bluefruit.h>
#include <nrf_soc.h>

#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>

#include "shared.h"

using namespace emu;

#define EMU_LOG(...) (fprintf(stderr, "[emu] " __VA_ARGS__), fputc('\n', stderr))

// ===== время =====
// Время виртуальное: его ведёт эмулятор (пауза, ускорение). Обращения к шине I2C
// "занимают" столько времени, сколько заняли бы на 400 кГц.

static thread_local uint64_t busy_debt_us = 0;

static void waitUntil(uint64_t target)
{
  for(;;)
  {
    uint64_t now = nowUs();
    if(now >= target) return;
    if(board->paused.load(std::memory_order_relaxed))
    {
      usleep(5000);
      continue;
    }
    double rate = board->rate_x1000.load(std::memory_order_relaxed) / 1000.0;
    uint64_t real = (uint64_t)((target - now) / (rate > 0 ? rate : 1));
    usleep(real < 20 ? 20 : (real > 2000 ? 2000 : real));
  }
}

static void busy(uint64_t us)
{
  busy_debt_us += us;
  if(busy_debt_us >= 1000)
  {
    waitUntil(nowUs() + busy_debt_us);
    busy_debt_us = 0;
  }
}

uint32_t millis()
{
  return (uint32_t)((nowUs() - board->boot_us.load()) / 1000);
}

uint32_t micros()
{
  return (uint32_t)(nowUs() - board->boot_us.load());
}

void delay(uint32_t ms)
{
  waitUntil(nowUs() + (uint64_t)ms * 1000 + busy_debt_us);
  busy_debt_us = 0;
}

void delayMicroseconds(uint32_t us)
{
  busy(us);
}

void yield()
{
  usleep(50);
}

// ===== GPIO =====

static bool checkPin(uint32_t pin, const char* fn)
{
  if(pin < (uint32_t)kPins) return true;
  EMU_LOG("%s(%u): нет такого вывода (на плате 0..%d)", fn, pin, kPins - 1);
  return false;
}

void pinMode(uint32_t pin, uint32_t mode)
{
  if(!checkPin(pin, "pinMode")) return;
  uint8_t m = PIN_Z;
  switch(mode)
  {
    case INPUT: m = PIN_IN; break;
    case OUTPUT: m = PIN_OUT; break;
    case INPUT_PULLUP: m = PIN_IN_PU; break;
    case INPUT_PULLDOWN: m = PIN_IN_PD; break;
    case INPUT_PULLUP_SENSE: m = PIN_IN_PU_SENSE; break;
    case INPUT_PULLDOWN_SENSE: m = PIN_IN_PD_SENSE; break;
    default: EMU_LOG("pinMode(%u, %u): неизвестный режим", pin, mode); return;
  }
  board->pin_mode[pin] = m;
}

void digitalWrite(uint32_t pin, uint32_t val)
{
  if(!checkPin(pin, "digitalWrite")) return;
  board->pin_out[pin] = val ? 1 : 0;
}

void digitalToggle(uint32_t pin)
{
  if(!checkPin(pin, "digitalToggle")) return;
  board->pin_out[pin] = !board->pin_out[pin];
}

int digitalRead(uint32_t pin)
{
  if(!checkPin(pin, "digitalRead")) return 0;
  int l = pinLevel(pin);
  return l < 0 ? 0 : l;
}

// ===== АЦП =====

static uint32_t adcRefMv()
{
  switch(board->adc_ref.load())
  {
    case ADC_REF_3_0: return 3000;
    case ADC_REF_2_4: return 2400;
    case ADC_REF_1_8: return 1800;
    case ADC_REF_1_2: return 1200;
    case ADC_REF_VDD4: return 3300;
    default: return 3600;
  }
}

static uint32_t adcConvert(double mv)
{
  uint32_t max_val = (1u << board->adc_bits.load()) - 1;
  double v = mv / adcRefMv() * (max_val + 1) + (rand() % 3 - 1); // шум ±1 МЗР
  uint32_t r = v < 0 ? 0 : (v > max_val ? max_val : (uint32_t)(v + 0.5));
  board->adc_last = r;
  board->adc_reads++;
  busy(board->adc_ref.load() == ADC_REF_DEFAULT ? 10 : 40);
  return r;
}

void analogReference(eAnalogReference ref)
{
  static const uint8_t table[] = {ADC_REF_DEFAULT, ADC_REF_DEFAULT, ADC_REF_3_0, ADC_REF_2_4,
                                  ADC_REF_1_8, ADC_REF_1_2, ADC_REF_VDD4};
  board->adc_ref = table[ref];
}

void analogReadResolution(int bits)
{
  board->adc_bits = (uint8_t)constrain(bits, 8, 14);
}

void analogSampleTime(uint8_t) {}

uint32_t analogRead(uint32_t pin)
{
  // PIN_VBAT: на плате нет делителя на этом выводе, читаем как будто он висит на VDD/2
  if(pin == PIN_VBAT) return adcConvert(board->vbat_mv.load() / 2.0);
  return adcConvert(0);
}

uint32_t analogReadVDD()
{
  return adcConvert(3300.0 / 6);
}

uint32_t analogReadVDDHDIV5()
{
  return adcConvert(board->vbat_mv.load() / 5.0);
}

// ===== прочее Arduino =====

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  if(in_max == in_min) return out_min;
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

long random(long max)
{
  return max <= 0 ? 0 : rand() % max;
}

long random(long min, long max)
{
  return min >= max ? min : min + random(max - min);
}

void randomSeed(unsigned long seed)
{
  if(seed) srand(seed);
}

uint32_t readResetReason()
{
  return board->reset_reason.load();
}

void initVariant()
{
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  pinMode(PIN_PWREN, OUTPUT);
  digitalWrite(PIN_PWREN, LOW);
}

size_t Print::print(double v, int digits)
{
  char buf[48];
  snprintf(buf, sizeof(buf), "%.*f", digits, v);
  return write(buf);
}

size_t Print::printNum(unsigned long v, int base)
{
  if(base < 2) base = 10;
  char buf[8 * sizeof(long) + 1];
  char* p = buf + sizeof(buf) - 1;
  *p = 0;
  do
  {
    int d = v % base;
    *--p = d < 10 ? '0' + d : 'A' + d - 10;
    v /= base;
  } while(v);
  return write(p);
}

size_t Print::printSigned(long v, int base)
{
  if(base == 10 && v < 0) return write((uint8_t)'-') + printNum(-(unsigned long)v, 10);
  return printNum((unsigned long)v, base);
}

size_t Print::printf(const char* fmt, ...)
{
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  return n > 0 ? write(buf) : 0;
}

HardwareSerial Serial;
HardwareSerial Serial1;
SPIClass SPI;

size_t HardwareSerial::write(uint8_t c)
{
  fputc(c, stdout);
  if(c == '\n') fflush(stdout);
  return 1;
}

void HardwareSerial::flush()
{
  fflush(stdout);
}

// ===== SH1106 =====

void emu::sh1106Reset(Sh1106& o)
{
  // после подачи питания в памяти кадра мусор, дисплей выключен
  for(int p = 0; p < 8; p++)
    for(int c = 0; c < 132; c++) o.ram[p][c] = (uint8_t)rand();
  o.display_on = 0;
  o.inverted = 0;
  o.entire_on = 0;
  o.seg_remap = 0;
  o.com_remap = 0;
  o.contrast = 0x80;
  o.start_line = 0;
  o.offset = 0;
  o.multiplex = 63;
  o.page = 0;
  o.col = 0;
  o.pending_cmd = 0;
  o.pending_args = 0;
}

static void sh1106Data(Sh1106& o, uint8_t d)
{
  o.data_bytes++;
  if(o.col < 132) o.ram[o.page & 7][o.col] = d;
  if(o.col < 131) o.col++;
}

static void sh1106Cmd(Sh1106& o, uint8_t c)
{
  o.cmd_bytes++;
  if(o.pending_args)
  {
    o.pending_args--;
    switch(o.pending_cmd)
    {
      case 0x81: o.contrast = c; break;
      case 0xA8: o.multiplex = c & 0x3F; break;
      case 0xD3: o.offset = c & 0x3F; break;
      default: break; // параметры, не влияющие на изображение
    }
    return;
  }

  if(c <= 0x0F) { o.col = (o.col & 0xF0) | c; return; }
  if(c <= 0x1F) { o.col = (o.col & 0x0F) | ((c & 0x0F) << 4); return; }
  if(c >= 0x40 && c <= 0x7F) { o.start_line = c & 0x3F; return; }
  if(c >= 0xB0 && c <= 0xB7) { o.page = c & 7; return; }
  if(c >= 0x30 && c <= 0x33) return; // напряжение накачки

  switch(c)
  {
    // команды с одним аргументом (часть из них - от SSD1306, GyverOLED шлёт их и для SH1106)
    case 0x20: case 0x81: case 0xA8: case 0xD3: case 0xD5: case 0xD9:
    case 0xDA: case 0xDB: case 0x8D: case 0xAD:
      o.pending_cmd = c;
      o.pending_args = 1;
      return;
    case 0x21: case 0x22: // окно SSD1306, у SH1106 таких команд нет
      o.pending_cmd = c;
      o.pending_args = 2;
      return;
    case 0xA0: o.seg_remap = 0; return;
    case 0xA1: o.seg_remap = 1; return;
    case 0xA4: o.entire_on = 0; return;
    case 0xA5: o.entire_on = 1; return;
    case 0xA6: o.inverted = 0; return;
    case 0xA7: o.inverted = 1; return;
    case 0xAE: o.display_on = 0; return;
    case 0xAF: o.display_on = 1; return;
    case 0xE0: case 0xEE: case 0xE3: return;
    default:
      if((c & 0xF0) == 0xC0) { o.com_remap = (c & 0x08) ? 1 : 0; return; }
      o.unknown_cmds++;
      o.last_unknown = c;
      EMU_LOG("SH1106: неизвестная команда 0x%02X", c);
  }
}

// одна транзакция записи: управляющий байт (Co, D/C) + данные
static void sh1106Write(const uint8_t* b, size_t n)
{
  Sh1106& o = board->oled;
  size_t i = 0;
  while(i < n)
  {
    uint8_t ctrl = b[i++];
    bool cont = ctrl & 0x80;
    bool data = ctrl & 0x40;
    if(!cont)
    {
      for(; i < n; i++) data ? sh1106Data(o, b[i]) : sh1106Cmd(o, b[i]);
    }
    else if(i < n)
    {
      data ? sh1106Data(o, b[i]) : sh1106Cmd(o, b[i]);
      i++;
    }
  }
}

static bool oledPowered()
{
  bool pwr = board->pin_mode[kPwrEnPin] == PIN_OUT && board->pin_out[kPwrEnPin];
  Sh1106& o = board->oled;
  if(pwr && !o.powered)
  {
    sh1106Reset(o);
    o.powered = 1;
  }
  if(!pwr) o.powered = 0;
  return pwr;
}

// ===== I2C =====

TwoWire Wire;

void TwoWire::begin()
{
  board->wire_begun = 1;
  board->pin_mode[kSdaPin] = PIN_I2C;
  board->pin_mode[kSclPin] = PIN_I2C;
  if(!board->i2c_clock) board->i2c_clock = 100000;
}

void TwoWire::end()
{
  board->wire_begun = 0;
  board->pin_mode[kSdaPin] = PIN_Z;
  board->pin_mode[kSclPin] = PIN_Z;
}

void TwoWire::setClock(uint32_t hz)
{
  board->i2c_clock = hz;
}

void TwoWire::beginTransmission(uint8_t addr)
{
  addr_ = addr;
  tx_len_ = 0;
  tx_overflow_ = false;
}

size_t TwoWire::write(uint8_t b)
{
  if(tx_len_ >= kBuf)
  {
    if(!tx_overflow_) EMU_LOG("Wire: переполнен буфер передачи (%zu байт), лишнее отброшено", kBuf);
    tx_overflow_ = true;
    return 0;
  }
  tx_[tx_len_++] = b;
  return 1;
}

size_t TwoWire::write(const uint8_t* buf, size_t n)
{
  size_t r = 0;
  while(n--) r += write(*buf++);
  return r;
}

static void busTime(size_t bytes)
{
  uint32_t clk = board->i2c_clock.load();
  if(!clk) clk = 100000;
  board->i2c_txn++;
  board->i2c_bytes += bytes;
  busy((bytes * 9 + 2) * 1000000ull / clk + 5);
}

uint8_t TwoWire::endTransmission(bool)
{
  if(!board->wire_begun)
  {
    static bool warned = false;
    if(!warned) EMU_LOG("Wire: передача до Wire.begin() или после Wire.end()");
    warned = true;
    return 4;
  }
  busTime(tx_len_ + 1);
  if(addr_ != kOledAddr || !oledPowered())
  {
    board->i2c_nack++;
    return 2; // NACK на адрес
  }
  sh1106Write(tx_, tx_len_);
  return 0;
}

uint8_t TwoWire::requestFrom(int addr, int qty, bool)
{
  rx_len_ = rx_pos_ = 0;
  if(!board->wire_begun) return 0;
  busTime(qty + 1);
  if(addr != kOledAddr || !oledPowered())
  {
    board->i2c_nack++;
    return 0;
  }
  Sh1106& o = board->oled;
  for(int i = 0; i < qty && rx_len_ < kBuf; i++)
    rx_[rx_len_++] = i == 0 ? 0 : o.ram[o.page & 7][o.col < 132 ? o.col : 131]; // первый байт - пустое чтение
  return rx_len_;
}

int TwoWire::available()
{
  return rx_len_ - rx_pos_;
}

int TwoWire::read()
{
  return rx_pos_ < rx_len_ ? rx_[rx_pos_++] : -1;
}

// ===== SoftDevice =====

uint32_t sd_softdevice_is_enabled(uint8_t* enabled)
{
  *enabled = board->sd_enabled.load();
  return NRF_SUCCESS;
}

uint32_t sd_power_system_off()
{
  if(!board->sd_enabled)
  {
    EMU_LOG("sd_power_system_off(): SoftDevice не включён - вызов не сработает");
    return NRF_ERROR_SOFTDEVICE_NOT_ENABLED;
  }
  fflush(stdout);
  fflush(stderr);
  _exit(kExitSystemOff);
}

uint32_t sd_power_usbdetected_enable(uint8_t enable)
{
  if(!board->sd_enabled) return NRF_ERROR_SOFTDEVICE_NOT_ENABLED;
  board->usbdetect_en = enable ? 1 : 0;
  return NRF_SUCCESS;
}

uint32_t sd_power_usbregstatus_get(uint32_t* status)
{
  if(!board->sd_enabled) return NRF_ERROR_SOFTDEVICE_NOT_ENABLED;
  *status = board->usb ? (POWER_USBREGSTATUS_VBUSDETECT_Msk | POWER_USBREGSTATUS_OUTPUTRDY_Msk) : 0;
  return NRF_SUCCESS;
}

uint32_t sd_rand_application_vector_get(uint8_t* buf, uint8_t len)
{
  if(!board->sd_enabled) return NRF_ERROR_SOFTDEVICE_NOT_ENABLED;
  while(len--) *buf++ = (uint8_t)rand();
  return NRF_SUCCESS;
}

// ===== Bluefruit =====

AdafruitBluefruit Bluefruit;
static BLECharacteristic* write_char = nullptr;

BLEService::BLEService(const uint8_t uuid128[16])
{
  memcpy(uuid, uuid128, 16);
}

void BLEService::begin() {}

BLECharacteristic::BLECharacteristic(const uint8_t uuid128[16])
{
  memcpy(uuid, uuid128, 16);
}

void BLECharacteristic::begin()
{
  if(props_ & (CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP))
  {
    if(write_char) EMU_LOG("Bluefruit: эмулятор поддерживает только одну записываемую характеристику");
    write_char = this;
    board->char_ready = write_perm_ != SECMODE_NO_ACCESS;
  }
}

void BLEDis::begin()
{
  EMU_LOG("BLE DIS: %s / %s, hw %s, fw %s", manufacturer, model, hw_rev, fw_rev);
}

void BLEPeriph::setConnIntervalMS(uint16_t min_ms, uint16_t max_ms)
{
  board->ci_min_ms = min_ms;
  board->ci_max_ms = max_ms;
}

void BLEAdvertising::restartOnDisconnect(bool enable)
{
  board->restart_on_disc = enable;
}

bool BLEAdvertising::start(uint16_t)
{
  if(!board->sd_enabled) return false;
  if(!board->connected) board->adv = 1;
  return true;
}

bool BLEAdvertising::stop()
{
  board->adv = 0;
  return true;
}

bool BLEAdvertising::isRunning()
{
  return board->adv;
}

bool AdafruitBluefruit::begin(uint8_t, uint8_t)
{
  board->sd_enabled = 1;
  board->tx_power = 0;
  return true;
}

void AdafruitBluefruit::setTxPower(int8_t power)
{
  board->tx_power = power;
}

bool AdafruitBluefruit::connected()
{
  return board->connected;
}

// ===== фоновый поток: события SoftDevice, светодиод связи, снимок памяти =====

__attribute__((no_sanitize("address"))) static void snapshotMemory()
{
  for(int i = 0; i < symbol_count; i++)
  {
    const Symbol& s = symbols[i];
    const volatile uint8_t* src = (const volatile uint8_t*)s.addr;
    for(uint32_t j = 0; j < s.size; j++) board->mem[s.mem_off + j] = src[j];
  }
  board->mem_seq++;
}

static void handleBle(const BleCmd& c)
{
  if(!board->sd_enabled) return; // радио выключено - телефон ничего не видит
  switch(c.type)
  {
    case BLE_CONNECT:
      if(!board->adv || board->connected) return;
      board->adv = 0;
      board->connected = 1;
      board->ble_conn_count++;
      if(Bluefruit.Periph.connect_cb) Bluefruit.Periph.connect_cb(0);
      break;
    case BLE_DISCONNECT:
      if(!board->connected) return;
      board->connected = 0;
      if(Bluefruit.Periph.disconnect_cb) Bluefruit.Periph.disconnect_cb(0, 0x13);
      if(board->restart_on_disc) board->adv = 1;
      break;
    case BLE_WRITE:
    {
      if(!board->connected || !write_char || !board->char_ready) return;
      if(c.len > write_char->max_len_)
      {
        board->ble_rejected++;
        EMU_LOG("BLE: запись %u байт отклонена стеком (max_len %u)", c.len, write_char->max_len_);
        return;
      }
      board->ble_rx++;
      uint8_t buf[64];
      memcpy(buf, c.data, c.len);
      if(write_char->write_cb_) write_char->write_cb_(0, write_char, buf, c.len);
      break;
    }
  }
}

static void* backgroundThread(void* arg)
{
  pid_t parent = (pid_t)(intptr_t)arg;
  uint64_t next_snap = 0;
  for(;;)
  {
    if(getppid() != parent) _exit(0); // эмулятор закрыли

    BleCmd c;
    while(board->ble_q.pop(c)) handleBle(c);

    uint64_t now = nowUs();
    // светодиод связи Bluefruit: мигает при рекламе, горит при подключении
    if(board->sd_enabled && Bluefruit.auto_led)
    {
      bool led = board->connected ? true : (board->adv ? ((now / 500000) & 1) : false);
      board->pin_out[kLedPin] = led;
    }

    if(now >= next_snap)
    {
      snapshotMemory();
      next_snap = now + 50000;
    }
    usleep(500);
  }
  return nullptr;
}

void setup();
void loop();

namespace emu
{
// точка входа процесса прошивки (вызывается после fork)
[[noreturn]] void firmwareMain(pid_t parent)
{
  setvbuf(stdout, nullptr, _IOLBF, 0);
  setvbuf(stderr, nullptr, _IOLBF, 0);
  srand((unsigned)(nowUs() ^ getpid()));

  pthread_t th;
  pthread_create(&th, nullptr, backgroundThread, (void*)(intptr_t)parent);

  initVariant();
  board->in_setup = 1;
  setup();
  board->in_setup = 0;
  for(;;)
  {
    loop();
    board->loop_count++;
    board->last_loop_us = nowUs();
  }
}
} // namespace emu
