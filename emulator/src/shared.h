#pragma once
// Состояние платы в разделяемой памяти: его пишет прошивка (дочерний процесс),
// а читает и меняет эмулятор (родительский процесс с веб-интерфейсом).
#include <atomic>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace emu
{

constexpr int kPins = 25;              // цифровые выводы платы (variant.h: PINS_COUNT)
constexpr int kButtonPin = 0;          // utils.cpp: button_pin
constexpr int kLedPin = 17;            // variant.h: PIN_LED
constexpr int kPwrEnPin = 22;          // variant.h: PIN_PWREN
constexpr int kSdaPin = 20;
constexpr int kSclPin = 21;
constexpr uint8_t kOledAddr = 0x3C;
constexpr int kExitSystemOff = 0x5F;   // код выхода процесса прошивки при System OFF
constexpr size_t kMemSize = 64 * 1024; // снимок переменных прошивки

// биты RESETREAS (nRF52840)
constexpr uint32_t kResetPin = 1u << 0;
constexpr uint32_t kResetOff = 1u << 16;  // пробуждение из System OFF по GPIO DETECT
constexpr uint32_t kResetVbus = 1u << 20; // пробуждение из System OFF по VBUS

enum PinMode : uint8_t
{
  PIN_Z = 0,       // не настроен (после сброса)
  PIN_IN,
  PIN_IN_PU,
  PIN_IN_PD,
  PIN_IN_PU_SENSE, // вход с подтяжкой и SENSE=LOW (будит из System OFF)
  PIN_IN_PD_SENSE, // SENSE=HIGH
  PIN_OUT,
  PIN_I2C,
};

enum CpuState : uint8_t
{
  CPU_OFF = 0,  // System OFF
  CPU_RUNNING,
  CPU_CRASHED,  // процесс прошивки упал (сигнал, assert, санитайзер)
};

enum AdcRef : uint8_t
{
  ADC_REF_DEFAULT = 0, // 3.6 В
  ADC_REF_3_0,
  ADC_REF_2_4,
  ADC_REF_1_8,
  ADC_REF_1_2,
  ADC_REF_VDD4,
};

enum BleCmdType : uint8_t
{
  BLE_CONNECT = 1,
  BLE_DISCONNECT,
  BLE_WRITE,
};

struct BleCmd
{
  uint8_t type;
  uint8_t len;
  uint8_t data[64];
};

// очередь событий от "телефона" к SoftDevice: один читатель (прошивка), писатели под мьютексом родителя
struct BleQueue
{
  static constexpr uint32_t N = 64;
  std::atomic<uint32_t> head;
  std::atomic<uint32_t> tail;
  BleCmd items[N];

  bool push(const BleCmd& c)
  {
    uint32_t h = head.load(std::memory_order_relaxed);
    if(h - tail.load(std::memory_order_acquire) >= N) return false;
    items[h % N] = c;
    head.store(h + 1, std::memory_order_release);
    return true;
  }

  bool pop(BleCmd& c)
  {
    uint32_t t = tail.load(std::memory_order_relaxed);
    if(t == head.load(std::memory_order_acquire)) return false;
    c = items[t % N];
    tail.store(t + 1, std::memory_order_release);
    return true;
  }

  void clear()
  {
    tail.store(head.load());
  }
};

// модель контроллера SH1106: память кадра 132x64 и регистры
struct Sh1106
{
  uint8_t ram[8][132];
  uint8_t powered;    // есть питание (PWREN)
  uint8_t display_on; // AE/AF
  uint8_t inverted;   // A6/A7
  uint8_t entire_on;  // A4/A5
  uint8_t seg_remap;  // A0/A1
  uint8_t com_remap;  // C0/C8
  uint8_t contrast;
  uint8_t start_line;
  uint8_t offset;
  uint8_t multiplex;
  uint8_t page;
  uint8_t col;
  uint8_t pending_cmd;  // команда, ждущая аргументов
  uint8_t pending_args;
  uint32_t data_bytes;
  uint32_t cmd_bytes;
  uint32_t unknown_cmds;
  uint8_t last_unknown;
};

struct Board
{
  // виртуальное время (пишет родитель)
  std::atomic<uint64_t> virt_us;
  std::atomic<uint32_t> rate_x1000;
  std::atomic<uint8_t> paused;

  // внешние воздействия (пишет родитель)
  std::atomic<uint8_t> button;   // 1 = кнопка нажата (замыкает вывод на землю)
  std::atomic<uint8_t> usb;      // 1 = есть VBUS
  std::atomic<uint32_t> vbat_mv; // напряжение на VDDH

  // процессор
  std::atomic<uint8_t> cpu;
  std::atomic<uint32_t> boot_count;
  std::atomic<uint32_t> reset_reason;
  std::atomic<uint64_t> boot_us;
  std::atomic<uint64_t> last_loop_us;
  std::atomic<uint32_t> loop_count;
  std::atomic<uint8_t> in_setup;

  // GPIO
  std::atomic<uint8_t> pin_mode[kPins];
  std::atomic<uint8_t> pin_out[kPins];

  // питание / SoftDevice
  std::atomic<uint8_t> sd_enabled;
  std::atomic<uint8_t> usbdetect_en;

  // АЦП
  std::atomic<uint8_t> adc_ref;
  std::atomic<uint8_t> adc_bits;
  std::atomic<uint32_t> adc_last;
  std::atomic<uint32_t> adc_reads;

  // I2C
  std::atomic<uint8_t> wire_begun;
  std::atomic<uint32_t> i2c_clock;
  std::atomic<uint32_t> i2c_txn;
  std::atomic<uint32_t> i2c_bytes;
  std::atomic<uint32_t> i2c_nack;
  Sh1106 oled;

  // BLE
  std::atomic<uint8_t> adv;
  std::atomic<uint8_t> connected;
  std::atomic<uint8_t> restart_on_disc;
  std::atomic<uint8_t> char_ready;
  std::atomic<int8_t> tx_power;
  std::atomic<uint16_t> ci_min_ms;
  std::atomic<uint16_t> ci_max_ms;
  std::atomic<uint32_t> ble_rx;
  std::atomic<uint32_t> ble_rejected;
  std::atomic<uint32_t> ble_conn_count;
  BleQueue ble_q;

  // снимок глобальных переменных прошивки
  std::atomic<uint32_t> mem_seq;
  uint8_t mem[kMemSize];
};

extern Board* board;

// переменная прошивки (из таблицы символов, см. tools/gen_syms.py)
struct Symbol
{
  uintptr_t addr;  // адрес в процессе (уже с поправкой на ASLR)
  uint32_t size;
  uint32_t mem_off; // смещение в board->mem
  char name[96];
  char file[32];
};
extern Symbol* symbols;
extern int symbol_count;

// уровень на выводе: 0, 1 или -1 (висит в воздухе)
inline int pinLevel(int p)
{
  if(p < 0 || p >= kPins) return -1;
  uint8_t m = board->pin_mode[p].load(std::memory_order_relaxed);
  if(m == PIN_OUT) return board->pin_out[p].load(std::memory_order_relaxed);
  if(m == PIN_I2C) return 1; // линии подтянуты к питанию
  if(p == kButtonPin && board->button.load(std::memory_order_relaxed)) return 0;
  if(m == PIN_IN_PU || m == PIN_IN_PU_SENSE) return 1;
  if(m == PIN_IN_PD || m == PIN_IN_PD_SENSE) return 0;
  return -1;
}

inline uint64_t nowUs()
{
  return board->virt_us.load(std::memory_order_relaxed);
}

void sh1106Reset(Sh1106& o);

} // namespace emu
