// Эмулятор платы IMBA TV: запускает прошивку, моделирует окружение
// (кнопка, USB, батарея, телефон с LoEUC) и показывает состояние платы в браузере.
//
// Процессы:
//   эмулятор (потоки: часы, супервизор, телефон, HTTP, сценарий)
//     └─ зигота: однопоточная, только порождает прошивку
//          └─ прошивка: setup()/loop(); каждая перезагрузка - новый процесс,
//             поэтому все static-переменные начинаются с нуля, как после сброса
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <fstream>
#include <map>
#include <mutex>
#include <new>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "shared.h"

using namespace emu;

namespace emu
{
Board* board = nullptr;
Symbol* symbols = nullptr;
int symbol_count = 0;
[[noreturn]] void firmwareMain(pid_t parent);
} // namespace emu

// метка для вычисления сдвига ASLR (см. tools/gen_syms.py)
extern "C" {
__attribute__((used)) volatile char emu_mem_anchor = 1;
}

#ifdef MSG_NOSIGNAL
#define MSG_NOSIGNAL_COMPAT MSG_NOSIGNAL
#else
#define MSG_NOSIGNAL_COMPAT 0 // macOS: SIGPIPE игнорируется в main()
#endif

static double constrainD(double v, double lo, double hi)
{
  return v < lo ? lo : (v > hi ? hi : v);
}

#ifndef EMU_WEB_DIR
#define EMU_WEB_DIR "web"
#endif

// ===== журнал =====

static std::mutex log_mtx;
static std::deque<std::string> log_lines;
static uint64_t log_first = 0; // номер первой строки в log_lines
static bool quiet = false;

static void logLine(const std::string& s)
{
  char ts[32];
  uint64_t t = board ? nowUs() : 0;
  snprintf(ts, sizeof(ts), "%7.3f ", t / 1e6);
  std::string line = ts + s;
  if(!quiet) fprintf(stderr, "%s\n", line.c_str());
  std::lock_guard<std::mutex> lk(log_mtx);
  log_lines.push_back(line);
  if(log_lines.size() > 3000)
  {
    log_lines.pop_front();
    log_first++;
  }
}

static void logf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
static void logf(const char* fmt, ...)
{
  char buf[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  logLine(std::string("[board] ") + buf);
}

// ===== таблица символов прошивки =====

static void loadSymbols(const std::string& path)
{
  std::ifstream f(path);
  if(!f)
  {
    fprintf(stderr, "нет таблицы символов %s - вкладка памяти будет пустой\n", path.c_str());
    return;
  }
  std::vector<Symbol> list;
  long long slide = 0;
  std::string line;
  uint32_t off = 0;
  while(std::getline(f, line))
  {
    std::vector<std::string> col;
    std::stringstream ss(line);
    std::string c;
    while(std::getline(ss, c, '\t')) col.push_back(c);
    if(col.size() >= 2 && col[0] == "#anchor")
    {
      slide = (long long)(uintptr_t)&emu_mem_anchor - (long long)strtoull(col[1].c_str(), nullptr, 16);
      continue;
    }
    if(col.size() < 5) continue;
    Symbol s{};
    s.addr = (uintptr_t)strtoull(col[0].c_str(), nullptr, 16);
    s.size = (uint32_t)strtoul(col[1].c_str(), nullptr, 10);
    if(s.size == 0 || off + s.size > kMemSize) continue;
    snprintf(s.name, sizeof(s.name), "%s", col[3].c_str());
    snprintf(s.file, sizeof(s.file), "%s", col[4].c_str());
    s.mem_off = off;
    off += s.size;
    list.push_back(s);
  }
  if(!slide)
  {
    fprintf(stderr, "в %s нет метки #anchor - таблица устарела, пересоберите\n", path.c_str());
    return;
  }
  symbols = new Symbol[list.size()];
  for(size_t i = 0; i < list.size(); i++)
  {
    symbols[i] = list[i];
    symbols[i].addr += slide;
  }
  symbol_count = (int)list.size();
}

// ===== зигота =====

struct ZMsg
{
  char type; // 'B' загрузка, 'K' убить; ответы: 'S' запущена, 'X' завершилась
  int32_t value;
};

static int zyg_cmd = -1, zyg_evt = -1;

[[noreturn]] static void zygoteMain(int cmd_fd, int evt_fd, int log_fd)
{
  pid_t parent = getppid();
  pid_t self = getpid();
  pid_t child = -1;
  for(;;)
  {
    if(getppid() != parent)
    {
      if(child > 0) kill(child, SIGKILL);
      _exit(0);
    }
    if(child > 0)
    {
      int st;
      if(waitpid(child, &st, WNOHANG) == child)
      {
        child = -1;
        ZMsg m{'X', st};
        (void)!write(evt_fd, &m, sizeof(m));
      }
    }
    pollfd p{cmd_fd, POLLIN, 0};
    if(poll(&p, 1, 2) <= 0) continue;
    ZMsg m;
    if(read(cmd_fd, &m, sizeof(m)) != sizeof(m)) _exit(0);
    if(m.type == 'K' && child > 0)
    {
      int st;
      kill(child, SIGKILL);
      waitpid(child, &st, 0);
      child = -1;
      ZMsg r{'X', -1};
      (void)!write(evt_fd, &r, sizeof(r));
    }
    else if(m.type == 'B' && child <= 0)
    {
      child = fork();
      if(child == 0)
      {
        close(cmd_fd);
        close(evt_fd);
        dup2(log_fd, 1);
        dup2(log_fd, 2);
        close(log_fd);
        firmwareMain(self);
      }
      ZMsg r{'S', child};
      (void)!write(evt_fd, &r, sizeof(r));
    }
  }
}

// ===== часы =====

static void clockThread()
{
  auto prev = std::chrono::steady_clock::now();
  for(;;)
  {
    std::this_thread::sleep_for(std::chrono::microseconds(200));
    auto now = std::chrono::steady_clock::now();
    uint64_t real = std::chrono::duration_cast<std::chrono::microseconds>(now - prev).count();
    prev = now;
    if(board->paused) continue;
    board->virt_us += real * board->rate_x1000.load() / 1000;
  }
}

static void waitVirt(uint64_t us)
{
  uint64_t target = nowUs() + us;
  while(nowUs() < target) std::this_thread::sleep_for(std::chrono::microseconds(500));
}

// ===== окружение: телефон и телеметрия =====

struct Field
{
  const char* key;
  const char* title;
  double value;
  bool present;
  int offset; // смещение в пакете LoEUC
};

struct Phone
{
  bool enabled = true;   // приложение LoEUC запущено и ищет устройство
  bool tx = true;        // слать телеметрию после подключения
  bool ride = false;     // имитация поездки
  uint32_t interval_ms = 100;
  uint8_t frame = 0;
  uint64_t connect_at = 0;
  uint64_t next_tx = 0;
  Field fields[7] = {
    {"speed", "скорость, км/ч", 25, true, 6},
    {"pwm", "ШИМ, %", 40, true, 8},
    {"volt", "напряжение, В", 100.8, true, 10},
    {"charge", "заряд колеса, %", 80, true, 12},
    {"temp", "температура, °C", 35, true, 14},
    {"trip", "пробег, км", 12.3, true, 16},
    {"limit", "порог ШИМ, %", 80, true, 18},
  };
  Field* field(const std::string& k)
  {
    for(auto& f : fields)
      if(k == f.key) return &f;
    return nullptr;
  }
};

static std::mutex env_mtx; // защищает phone и запись в ble_q
static Phone phone;

static void blePush(const BleCmd& c)
{
  if(!board->ble_q.push(c)) logf("очередь BLE переполнена, событие потеряно");
}

static BleCmd buildPacket()
{
  BleCmd c{};
  c.type = BLE_WRITE;
  c.len = 20;
  c.data[0] = 'L';
  c.data[1] = 'E';
  c.data[2] = 1;
  c.data[3] = phone.frame++;
  for(auto& f : phone.fields)
  {
    int16_t v = INT16_MIN;
    if(f.present) v = (int16_t)lround(constrainD(f.value * 10, -32767, 32767));
    c.data[f.offset] = (uint16_t)v >> 8;
    c.data[f.offset + 1] = (uint16_t)v & 0xFF;
  }
  return c;
}

static void rideStep(double dt)
{
  static double t = 0;
  t += dt;
  double speed = 32 + 18 * sin(t / 6.0) + 8 * sin(t / 1.7);
  double accel = 18 * cos(t / 6.0) / 6.0 + 8 * cos(t / 1.7) / 1.7;
  if(speed < 0) speed = 0;
  phone.field("speed")->value = round(speed * 10) / 10;
  phone.field("pwm")->value = constrainD(round(speed * 1.6 + accel * 6), 0, 100);
  Field* volt = phone.field("volt");
  Field* charge = phone.field("charge");
  charge->value = constrainD(charge->value - dt * 0.02, 0, 100);
  volt->value = round((84 + charge->value * 0.168 - phone.field("pwm")->value * 0.05) * 10) / 10;
  phone.field("trip")->value += speed * dt / 3600;
  phone.field("temp")->value = constrainD(phone.field("temp")->value + (phone.field("pwm")->value - 40) * dt * 0.002, 20, 80);
}

static void phoneThread()
{
  uint64_t prev = nowUs();
  for(;;)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::lock_guard<std::mutex> lk(env_mtx);
    uint64_t now = nowUs();
    double dt = (now - prev) / 1e6;
    prev = now;
    if(phone.ride) rideStep(dt);

    bool running = board->cpu == CPU_RUNNING;
    if(running && phone.enabled && board->adv && !board->connected)
    {
      // телефон находит устройство и подключается примерно через полсекунды
      if(!phone.connect_at) phone.connect_at = now + 600000;
      else if(now >= phone.connect_at)
      {
        BleCmd c{};
        c.type = BLE_CONNECT;
        blePush(c);
        phone.connect_at = now + 2000000; // если не вышло - повтор
      }
    }
    else phone.connect_at = 0;

    if(running && board->connected && phone.tx && now >= phone.next_tx)
    {
      blePush(buildPacket());
      phone.next_tx = now + phone.interval_ms * 1000ull;
    }
  }
}

// ===== супервизор: питание, сброс, пробуждение =====

static std::atomic<int> pending_reset{-1}; // -1 нет; иначе причина сброса
static std::atomic<bool> crashed_flag{false};
static std::mutex crash_mtx;
static std::string crash_reason;
static pid_t fw_pid = -1;

static const char* resetReasonText(uint32_t r)
{
  if(r & kResetVbus) return "VBUS (пробуждение по USB)";
  if(r & kResetOff) return "GPIO (пробуждение кнопкой)";
  if(r & kResetPin) return "вывод RESET";
  return "включение питания";
}

static bool zygRead(ZMsg& m, int timeout_ms)
{
  pollfd p{zyg_evt, POLLIN, 0};
  if(poll(&p, 1, timeout_ms) <= 0) return false;
  return read(zyg_evt, &m, sizeof(m)) == sizeof(m);
}

static void boot(uint32_t reason)
{
  for(int i = 0; i < kPins; i++)
  {
    board->pin_mode[i] = PIN_Z;
    board->pin_out[i] = 0;
  }
  board->sd_enabled = 0;
  board->usbdetect_en = 0;
  board->wire_begun = 0;
  board->adc_ref = ADC_REF_DEFAULT;
  board->adc_bits = 10;
  board->adv = 0;
  board->connected = 0;
  board->char_ready = 0;
  board->restart_on_disc = 0;
  board->ble_q.clear();
  board->reset_reason = reason;
  board->boot_us = nowUs();
  board->last_loop_us = nowUs();
  board->loop_count = 0;
  board->boot_count++;
  board->cpu = CPU_RUNNING;
  {
    std::lock_guard<std::mutex> lk(crash_mtx);
    crash_reason.clear();
  }
  logf("загрузка #%u, причина: %s", board->boot_count.load(), resetReasonText(reason));
  ZMsg m{'B', 0};
  (void)!write(zyg_cmd, &m, sizeof(m));
  if(zygRead(m, 2000) && m.type == 'S') fw_pid = m.value;
}

static void killFirmware()
{
  if(board->cpu != CPU_RUNNING) return;
  ZMsg m{'K', 0};
  (void)!write(zyg_cmd, &m, sizeof(m));
  while(zygRead(m, 2000) && m.type != 'X') {}
  fw_pid = -1;
}

static void onFirmwareExit(int st)
{
  fw_pid = -1;
  if(WIFEXITED(st) && WEXITSTATUS(st) == kExitSystemOff)
  {
    board->cpu = CPU_OFF;
    logf("System OFF (пробуждение: %s%s)",
         (board->pin_mode[kButtonPin] == PIN_IN_PU_SENSE) ? "кнопка" : "кнопка отключена",
         board->usbdetect_en ? ", USB" : "");
    return;
  }
  char buf[128];
  if(WIFSIGNALED(st)) snprintf(buf, sizeof(buf), "сигнал %d (%s)", WTERMSIG(st), strsignal(WTERMSIG(st)));
  else snprintf(buf, sizeof(buf), "выход с кодом %d", WEXITSTATUS(st));
  {
    std::lock_guard<std::mutex> lk(crash_mtx);
    crash_reason = buf;
  }
  board->cpu = CPU_CRASHED;
  crashed_flag = true;
  logf("ПРОШИВКА УПАЛА: %s", buf);
}

static void supervisorThread()
{
  bool usb_prev = board->usb;
  boot(0);
  for(;;)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    ZMsg m;
    while(zygRead(m, 0))
      if(m.type == 'X' && m.value != -1) onFirmwareExit(m.value);

    int r = pending_reset.exchange(-1);
    if(r >= 0)
    {
      killFirmware();
      if(r == 0) // холодный сброс: снимаем питание со всего
      {
        for(int i = 0; i < kPins; i++) board->pin_mode[i] = PIN_Z;
        board->oled.powered = 0;
      }
      boot((uint32_t)r);
      continue;
    }

    // без PWREN дисплей обесточен
    if(!(board->pin_mode[kPwrEnPin] == PIN_OUT && board->pin_out[kPwrEnPin])) board->oled.powered = 0;

    bool usb = board->usb;
    if(board->cpu == CPU_OFF)
    {
      uint8_t mode = board->pin_mode[kButtonPin];
      int lvl = pinLevel(kButtonPin);
      if((mode == PIN_IN_PU_SENSE && lvl == 0) || (mode == PIN_IN_PD_SENSE && lvl == 1)) boot(kResetOff);
      else if(usb && !usb_prev && board->usbdetect_en) boot(kResetVbus);
    }
    usb_prev = usb;
  }
}

static void logReaderThread(int fd)
{
  std::string acc;
  char buf[4096];
  for(;;)
  {
    ssize_t n = read(fd, buf, sizeof(buf));
    if(n <= 0)
    {
      if(n < 0 && errno == EINTR) continue;
      return;
    }
    acc.append(buf, n);
    size_t pos;
    while((pos = acc.find('\n')) != std::string::npos)
    {
      std::string line = acc.substr(0, pos);
      if(!line.empty() && line.back() == '\r') line.pop_back();
      acc.erase(0, pos + 1);
      logLine(line.rfind("[emu]", 0) == 0 ? line : "[fw] " + line);
    }
  }
}

// ===== кадр дисплея =====

// что видно на стекле: 128x64, 1 = пиксель светится
static void renderGlass(uint8_t out[64][128])
{
  const Sh1106& o = board->oled;
  bool visible = o.powered && o.display_on;
  for(int y = 0; y < 64; y++)
  {
    int row = o.com_remap ? y : 63 - y;
    int line = (row + o.start_line + o.offset) & 63;
    for(int x = 0; x < 128; x++)
    {
      int col = o.seg_remap ? x + 2 : 129 - x;
      bool lit = (o.ram[line >> 3][col] >> (line & 7)) & 1;
      if(o.inverted) lit = !lit;
      if(o.entire_on) lit = true;
      if(y > o.multiplex) lit = false;
      out[y][x] = visible && lit;
    }
  }
}

static uint32_t crc32(const uint8_t* d, size_t n, uint32_t c = 0)
{
  c = ~c;
  for(size_t i = 0; i < n; i++)
  {
    c ^= d[i];
    for(int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0 - (c & 1)));
  }
  return ~c;
}

// PNG без сжатия (stored deflate) - без внешних библиотек
static bool writePng(const std::string& path, int scale)
{
  uint8_t glass[64][128];
  renderGlass(glass);
  int w = 128 * scale, h = 64 * scale;
  std::vector<uint8_t> raw;
  for(int y = 0; y < h; y++)
  {
    raw.push_back(0);
    for(int x = 0; x < w; x++) raw.push_back(glass[y / scale][x / scale] ? 0xFF : 0x10);
  }
  std::vector<uint8_t> z = {0x78, 0x01};
  uint32_t a = 1, b = 0;
  for(uint8_t v : raw)
  {
    a = (a + v) % 65521;
    b = (b + a) % 65521;
  }
  for(size_t pos = 0; pos < raw.size();)
  {
    size_t n = std::min<size_t>(65535, raw.size() - pos);
    z.push_back(pos + n == raw.size());
    z.push_back(n & 0xFF);
    z.push_back(n >> 8);
    z.push_back(~n & 0xFF);
    z.push_back((~n >> 8) & 0xFF);
    z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
    pos += n;
  }
  uint32_t adler = (b << 16) | a;
  for(int i = 3; i >= 0; i--) z.push_back(adler >> (i * 8));

  FILE* f = fopen(path.c_str(), "wb");
  if(!f) return false;
  auto be32 = [&](uint32_t v) { uint8_t t[4] = {(uint8_t)(v >> 24), (uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v}; fwrite(t, 1, 4, f); };
  auto chunk = [&](const char* type, const std::vector<uint8_t>& data) {
    be32(data.size());
    std::vector<uint8_t> td(type, type + 4);
    td.insert(td.end(), data.begin(), data.end());
    fwrite(td.data(), 1, td.size(), f);
    be32(crc32(td.data(), td.size()));
  };
  static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
  fwrite(sig, 1, 8, f);
  std::vector<uint8_t> ihdr = {(uint8_t)(w >> 24), (uint8_t)(w >> 16), (uint8_t)(w >> 8), (uint8_t)w,
                               (uint8_t)(h >> 24), (uint8_t)(h >> 16), (uint8_t)(h >> 8), (uint8_t)h,
                               8, 0, 0, 0, 0};
  chunk("IHDR", ihdr);
  chunk("IDAT", z);
  chunk("IEND", {});
  fclose(f);
  return true;
}

// ===== команды (общие для веб-интерфейса и сценариев) =====

static bool parseBool(const std::string& v)
{
  return v == "1" || v == "on" || v == "true" || v == "yes";
}

// возвращает текст ошибки или пустую строку
static std::string applyCmd(const std::string& k, const std::string& v)
{
  if(k == "button") board->button = parseBool(v);
  else if(k == "usb") board->usb = parseBool(v);
  else if(k == "vbat")
  {
    double mv = atof(v.c_str());
    if(mv < 50) mv *= 1000; // можно в вольтах
    board->vbat_mv = (uint32_t)constrainD(mv, 0, 5500);
  }
  else if(k == "pause") board->paused = parseBool(v);
  else if(k == "rate") board->rate_x1000 = (uint32_t)(constrainD(atof(v.c_str()), 0.05, 20) * 1000);
  else if(k == "reset")
  {
    if(v == "cold") pending_reset = 0;
    else pending_reset = (int)kResetPin;
  }
  else
  {
    std::lock_guard<std::mutex> lk(env_mtx);
    if(k == "phone")
    {
      phone.enabled = parseBool(v);
      if(!phone.enabled && board->connected)
      {
        BleCmd c{};
        c.type = BLE_DISCONNECT;
        blePush(c);
      }
    }
    else if(k == "drop") // обрыв связи: телефон переподключится сам
    {
      BleCmd c{};
      c.type = BLE_DISCONNECT;
      blePush(c);
    }
    else if(k == "tx") phone.tx = parseBool(v);
    else if(k == "ride") phone.ride = parseBool(v);
    else if(k == "interval") phone.interval_ms = (uint32_t)constrainD(atof(v.c_str()), 5, 10000);
    else if(k == "packet")
    {
      BleCmd c{};
      c.type = BLE_WRITE;
      std::string hex;
      for(char ch : v)
        if(isxdigit((unsigned char)ch)) hex += ch;
      if(hex.size() % 2 || hex.size() / 2 > sizeof(c.data)) return "packet: нужно чётное число hex-цифр, до 64 байт";
      c.len = hex.size() / 2;
      for(size_t i = 0; i < c.len; i++) c.data[i] = (uint8_t)strtoul(hex.substr(i * 2, 2).c_str(), nullptr, 16);
      if(!board->connected) return "packet: телефон не подключён";
      blePush(c);
    }
    else if(Field* f = phone.field(k))
    {
      if(v == "-" || v == "none") f->present = false;
      else
      {
        f->present = true;
        f->value = atof(v.c_str());
      }
    }
    else return "неизвестная команда: " + k;
  }
  return "";
}

static std::string applyCmdLine(const std::string& line)
{
  std::stringstream ss(line);
  std::string tok;
  while(ss >> tok)
  {
    size_t eq = tok.find('=');
    if(eq == std::string::npos) return "ожидается ключ=значение: " + tok;
    std::string err = applyCmd(tok.substr(0, eq), tok.substr(eq + 1));
    if(!err.empty()) return err;
  }
  return "";
}

// ===== JSON =====

static void hexInto(std::string& s, const uint8_t* d, size_t n)
{
  static const char* hx = "0123456789abcdef";
  s.reserve(s.size() + n * 2);
  for(size_t i = 0; i < n; i++)
  {
    s += hx[d[i] >> 4];
    s += hx[d[i] & 15];
  }
}

static std::string jsonEsc(const std::string& in)
{
  std::string o;
  for(unsigned char c : in)
  {
    if(c == '"' || c == '\\') { o += '\\'; o += c; }
    else if(c == '\n') o += "\\n";
    else if(c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); o += b; }
    else o += c;
  }
  return o;
}

static const char* pinModeName(uint8_t m)
{
  static const char* names[] = {"Z", "INPUT", "INPUT_PULLUP", "INPUT_PULLDOWN", "INPUT_PULLUP_SENSE",
                                "INPUT_PULLDOWN_SENSE", "OUTPUT", "I2C"};
  return m < sizeof(names) / sizeof(*names) ? names[m] : "?";
}

static const char* nrfPin[kPins] = {"P0.08", "P0.06", "P0.17", "P0.20", "P0.22", "P0.24", "P1.00", "P0.11",
                                    "P1.04", "P1.06", "P0.09", "P1.01", "P1.02", "P1.07", "P1.11", "P1.13",
                                    "P0.10", "P0.15", "P1.15", "P0.02", "P0.29", "P0.31", "P0.13", "P0.04",
                                    "P0.27"};

static const char* pinFunction(int p)
{
  switch(p)
  {
    case 0: return "кнопка";
    case 17: return "светодиод";
    case 20: return "SDA дисплея";
    case 21: return "SCL дисплея";
    case 22: return "PWREN (питание дисплея)";
    case 23: return "A2 / VBAT";
    case 24: return "LED_BLUE (никуда не подключён)";
    case 1: return "RX";
    default: return "";
  }
}

static std::string stateJson()
{
  std::string s = "{";
  char b[512];
  uint64_t now = nowUs();
  uint8_t cpu = board->cpu;
  std::string crash;
  {
    std::lock_guard<std::mutex> lk(crash_mtx);
    crash = crash_reason;
  }
  snprintf(b, sizeof(b),
           "\"t\":%.3f,\"rate\":%.3f,\"paused\":%d,\"cpu\":\"%s\",\"crash\":\"%s\",\"boot\":%u,"
           "\"reset_reason\":\"%s\",\"uptime\":%.3f,\"loops\":%u,\"loop_age\":%.3f,\"in_setup\":%d,",
           now / 1e6, board->rate_x1000 / 1000.0, (int)board->paused,
           cpu == CPU_RUNNING ? "running" : (cpu == CPU_OFF ? "off" : "crashed"), jsonEsc(crash).c_str(),
           board->boot_count.load(), resetReasonText(board->reset_reason),
           cpu == CPU_RUNNING ? (now - board->boot_us) / 1e6 : 0.0, board->loop_count.load(),
           cpu == CPU_RUNNING ? (now - board->last_loop_us) / 1e6 : 0.0, (int)board->in_setup);
  s += b;

  snprintf(b, sizeof(b), "\"in\":{\"button\":%d,\"usb\":%d,\"vbat\":%u},", (int)board->button, (int)board->usb,
           board->vbat_mv.load());
  s += b;

  {
    std::lock_guard<std::mutex> lk(env_mtx);
    snprintf(b, sizeof(b), "\"phone\":{\"enabled\":%d,\"tx\":%d,\"ride\":%d,\"interval\":%u,\"frame\":%u,\"fields\":[",
             phone.enabled, phone.tx, phone.ride, phone.interval_ms, phone.frame);
    s += b;
    for(int i = 0; i < 7; i++)
    {
      const Field& f = phone.fields[i];
      snprintf(b, sizeof(b), "%s{\"key\":\"%s\",\"title\":\"%s\",\"value\":%.1f,\"present\":%d}", i ? "," : "", f.key,
               f.title, f.value, f.present);
      s += b;
    }
    s += "]},";
  }

  s += "\"pins\":[";
  for(int i = 0; i < kPins; i++)
  {
    snprintf(b, sizeof(b), "%s{\"n\":%d,\"nrf\":\"%s\",\"fn\":\"%s\",\"mode\":\"%s\",\"out\":%d,\"lvl\":%d}",
             i ? "," : "", i, nrfPin[i], pinFunction(i), pinModeName(board->pin_mode[i]), (int)board->pin_out[i],
             pinLevel(i));
    s += b;
  }
  s += "],";

  uint32_t ref_mv[] = {3600, 3000, 2400, 1800, 1200, 3300};
  snprintf(b, sizeof(b),
           "\"power\":{\"sd\":%d,\"usbdetect\":%d,\"pwren\":%d,\"led\":%d},"
           "\"adc\":{\"ref_mv\":%u,\"bits\":%u,\"last\":%u,\"reads\":%u},"
           "\"i2c\":{\"begun\":%d,\"clock\":%u,\"txn\":%u,\"bytes\":%u,\"nack\":%u},"
           "\"ble\":{\"adv\":%d,\"conn\":%d,\"char\":%d,\"restart\":%d,\"txp\":%d,\"ci_min\":%u,\"ci_max\":%u,"
           "\"rx\":%u,\"rejected\":%u,\"conns\":%u},",
           (int)board->sd_enabled, (int)board->usbdetect_en, pinLevel(kPwrEnPin) == 1, pinLevel(kLedPin) == 1,
           ref_mv[board->adc_ref % 6], board->adc_bits.load(), board->adc_last.load(), board->adc_reads.load(),
           (int)board->wire_begun, board->i2c_clock.load(), board->i2c_txn.load(), board->i2c_bytes.load(),
           board->i2c_nack.load(), (int)board->adv, (int)board->connected, (int)board->char_ready,
           (int)board->restart_on_disc, (int)board->tx_power, board->ci_min_ms.load(), board->ci_max_ms.load(),
           board->ble_rx.load(), board->ble_rejected.load(), board->ble_conn_count.load());
  s += b;

  const Sh1106& o = board->oled;
  snprintf(b, sizeof(b),
           "\"oled\":{\"powered\":%d,\"on\":%d,\"inverted\":%d,\"entire_on\":%d,\"contrast\":%u,\"seg_remap\":%d,"
           "\"com_remap\":%d,\"start_line\":%u,\"offset\":%u,\"multiplex\":%u,\"page\":%u,\"col\":%u,"
           "\"data_bytes\":%u,\"cmd_bytes\":%u,\"unknown\":%u,\"glass\":\"",
           o.powered, o.display_on, o.inverted, o.entire_on, o.contrast, o.seg_remap, o.com_remap, o.start_line,
           o.offset, o.multiplex, o.page, o.col, o.data_bytes, o.cmd_bytes, o.unknown_cmds);
  s += b;
  uint8_t glass[64][128];
  renderGlass(glass);
  uint8_t packed[1024];
  for(int i = 0; i < 1024; i++)
  {
    uint8_t v = 0;
    for(int k = 0; k < 8; k++) v |= glass[i / 16][(i % 16) * 8 + k] << (7 - k);
    packed[i] = v;
  }
  hexInto(s, packed, sizeof(packed));
  s += "\"}}";
  return s;
}

static std::string memJson()
{
  std::string s = "{\"seq\":";
  s += std::to_string(board->mem_seq.load());
  s += ",\"gddram\":\"";
  hexInto(s, &board->oled.ram[0][0], sizeof(board->oled.ram));
  s += "\",\"syms\":[";
  for(int i = 0; i < symbol_count; i++)
  {
    const Symbol& sym = symbols[i];
    char b[256];
    snprintf(b, sizeof(b), "%s{\"name\":\"%s\",\"file\":\"%s\",\"size\":%u,\"hex\":\"", i ? "," : "",
             jsonEsc(sym.name).c_str(), jsonEsc(sym.file).c_str(), sym.size);
    s += b;
    hexInto(s, board->mem + sym.mem_off, sym.size);
    s += "\"}";
  }
  s += "]}";
  return s;
}

static std::string logJson(uint64_t since)
{
  std::lock_guard<std::mutex> lk(log_mtx);
  if(since < log_first) since = log_first;
  std::string s = "{\"next\":" + std::to_string(log_first + log_lines.size()) + ",\"lines\":[";
  for(uint64_t i = since; i < log_first + log_lines.size(); i++)
  {
    if(i > since) s += ",";
    s += "\"" + jsonEsc(log_lines[i - log_first]) + "\"";
  }
  return s + "]}";
}

// ===== HTTP =====

static std::string urlDecode(const std::string& in)
{
  std::string o;
  for(size_t i = 0; i < in.size(); i++)
  {
    if(in[i] == '+') o += ' ';
    else if(in[i] == '%' && i + 2 < in.size())
    {
      o += (char)strtoul(in.substr(i + 1, 2).c_str(), nullptr, 16);
      i += 2;
    }
    else o += in[i];
  }
  return o;
}

static void httpReply(int fd, int code, const char* type, const std::string& body)
{
  char hdr[256];
  snprintf(hdr, sizeof(hdr),
           "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nCache-Control: no-store\r\n"
           "Connection: close\r\n\r\n",
           code, code == 200 ? "OK" : "Error", type, body.size());
  std::string out = hdr + body;
  size_t off = 0;
  while(off < out.size())
  {
    ssize_t n = send(fd, out.data() + off, out.size() - off, MSG_NOSIGNAL_COMPAT);
    if(n <= 0) break;
    off += n;
  }
}

static void httpHandle(int fd)
{
  std::string req;
  char buf[4096];
  while(req.find("\r\n\r\n") == std::string::npos && req.size() < 16384)
  {
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    if(n <= 0) return;
    req.append(buf, n);
  }
  std::string target;
  {
    std::stringstream ss(req);
    std::string method;
    ss >> method >> target;
  }
  std::string path = target, query;
  size_t q = target.find('?');
  if(q != std::string::npos)
  {
    path = target.substr(0, q);
    query = target.substr(q + 1);
  }
  std::map<std::string, std::string> args;
  std::vector<std::pair<std::string, std::string>> ordered;
  std::stringstream qs(query);
  std::string kv;
  while(std::getline(qs, kv, '&'))
  {
    size_t eq = kv.find('=');
    std::string k = urlDecode(kv.substr(0, eq));
    std::string v = eq == std::string::npos ? "" : urlDecode(kv.substr(eq + 1));
    args[k] = v;
    ordered.push_back({k, v});
  }

  if(path == "/" || path == "/index.html")
  {
    std::ifstream f(std::string(EMU_WEB_DIR) + "/index.html");
    std::stringstream ss;
    ss << f.rdbuf();
    httpReply(fd, f ? 200 : 404, "text/html; charset=utf-8", f ? ss.str() : "index.html не найден");
  }
  else if(path == "/api/state") httpReply(fd, 200, "application/json", stateJson());
  else if(path == "/api/mem") httpReply(fd, 200, "application/json", memJson());
  else if(path == "/api/log") httpReply(fd, 200, "application/json", logJson(strtoull(args["since"].c_str(), nullptr, 10)));
  else if(path == "/api/cmd")
  {
    std::string err;
    for(auto& p : ordered)
    {
      err = applyCmd(p.first, p.second);
      if(!err.empty()) break;
    }
    httpReply(fd, err.empty() ? 200 : 400, "application/json",
              err.empty() ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"" + jsonEsc(err) + "\"}");
  }
  else if(path == "/favicon.ico") httpReply(fd, 204, "image/x-icon", "");
  else httpReply(fd, 404, "text/plain", "not found");
}

static void httpThread(int listen_fd)
{
  for(;;)
  {
    int fd = accept(listen_fd, nullptr, nullptr);
    if(fd < 0) continue;
    httpHandle(fd);
    close(fd);
  }
}

// ===== сценарии =====

static int script_failures = 0;

// expect glass=FILE: экран совпадает с картинкой из FILE - 64 строки по 128 знаков,
// '#' точка светится, '.' не светится, '?' не проверяется (например, заряд платы: он зависит от шума АЦП).
// Возвращает текст ошибки или пустую строку; при расхождении кадр пишется рядом в FILE.actual
static std::string glassCompare(const std::string& path)
{
  std::ifstream f(path);
  if(!f) return "нет файла " + path;
  std::vector<std::string> rows;
  std::string line;
  while(std::getline(f, line))
  {
    if(!line.empty() && line.back() == '\r') line.pop_back();
    if(!line.empty()) rows.push_back(line);
  }
  if(rows.size() != 64) return path + ": нужно 64 строки, а не " + std::to_string(rows.size());
  for(int y = 0; y < 64; y++)
    if(rows[y].size() != 128 || rows[y].find_first_not_of("#.?") != std::string::npos)
      return path + ": в строке " + std::to_string(y + 1) + " нужно 128 знаков из #, . и ?";

  uint8_t glass[64][128];
  renderGlass(glass);
  int diff = 0, fx = 0, fy = 0;
  for(int y = 0; y < 64; y++)
    for(int x = 0; x < 128; x++)
    {
      char want = rows[y][x];
      if(want == '?' || (want == '#') == (glass[y][x] != 0)) continue;
      if(!diff++) { fx = x; fy = y; }
    }
  if(!diff) return "";

  std::ofstream out(path + ".actual");
  for(int y = 0; y < 64; y++)
  {
    for(int x = 0; x < 128; x++) out << (glass[y][x] ? '#' : '.');
    out << '\n';
  }
  char b[160];
  snprintf(b, sizeof(b), "отличаются %d точек, первая x=%d y=%d; кадр записан в %s.actual", diff, fx, fy, path.c_str());
  return b;
}

static void scriptRun(const std::string& path)
{
  std::ifstream f(path);
  if(!f)
  {
    logf("сценарий %s не найден", path.c_str());
    script_failures++;
    return;
  }
  std::string line;
  int ln = 0;
  while(std::getline(f, line))
  {
    ln++;
    size_t hash = line.find('#');
    if(hash != std::string::npos) line = line.substr(0, hash);
    std::stringstream ss(line);
    std::string cmd;
    if(!(ss >> cmd)) continue;
    std::string rest;
    std::getline(ss, rest);
    while(!rest.empty() && rest[0] == ' ') rest.erase(0, 1);

    if(cmd == "wait") waitVirt((uint64_t)(atof(rest.c_str()) * 1000));
    else if(cmd == "echo") logLine("[script] " + rest);
    else if(cmd == "shot")
    {
      if(writePng(rest, 4)) logLine("[script] снимок экрана: " + rest);
      else { logLine("[script] не удалось записать " + rest); script_failures++; }
    }
    else if(cmd == "expect")
    {
      // expect cpu=running|off|crashed, expect connected=0|1, expect display=on|off, expect glass=FILE
      size_t eq = rest.find('=');
      std::string k = rest.substr(0, eq), v = eq == std::string::npos ? "" : rest.substr(eq + 1);
      if(k == "glass")
      {
        std::string err = glassCompare(v);
        if(!err.empty())
        {
          script_failures++;
          logf("строка %d: экран не совпал с %s: %s", ln, v.c_str(), err.c_str());
        }
        else logLine("[script] ok: " + rest);
        continue;
      }
      std::string actual;
      if(k == "cpu") actual = board->cpu == CPU_RUNNING ? "running" : (board->cpu == CPU_OFF ? "off" : "crashed");
      else if(k == "connected") actual = board->connected ? "1" : "0";
      else if(k == "display") actual = (board->oled.powered && board->oled.display_on) ? "on" : "off";
      else if(k == "led") actual = pinLevel(kLedPin) == 1 ? "1" : "0";
      else actual = "?";
      if(actual != v)
      {
        script_failures++;
        logf("строка %d: ожидалось %s=%s, на самом деле %s", ln, k.c_str(), v.c_str(), actual.c_str());
      }
      else logLine("[script] ok: " + rest);
    }
    else
    {
      std::string err = applyCmdLine(line);
      if(!err.empty())
      {
        script_failures++;
        logf("сценарий, строка %d: %s", ln, err.c_str());
      }
    }
  }
}

// ===== main =====

static void usage()
{
  fprintf(stderr,
          "Эмулятор платы IMBA TV\n"
          "  --port N        порт веб-интерфейса (по умолчанию 8080)\n"
          "  --script FILE   выполнить сценарий (см. emulator/README.md)\n"
          "  --headless      без веб-интерфейса; выйти после сценария (код 1 при ошибках)\n"
          "  --rate X        скорость времени (1 = реальное)\n"
          "  --usb           стартовать с подключённым USB\n"
          "  --vbat MV       напряжение батареи, мВ (по умолчанию 3900)\n"
          "  --quiet         не дублировать журнал в терминал\n");
}

int main(int argc, char** argv)
{
  int port = 8080;
  std::string script;
  bool headless = false;
  double rate = 1.0;
  bool usb = false;
  uint32_t vbat = 3900;
  for(int i = 1; i < argc; i++)
  {
    std::string a = argv[i];
    auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
    if(a == "--port") port = atoi(next().c_str());
    else if(a == "--script") script = next();
    else if(a == "--headless") headless = true;
    else if(a == "--rate") rate = atof(next().c_str());
    else if(a == "--usb") usb = true;
    else if(a == "--vbat") vbat = atoi(next().c_str());
    else if(a == "--quiet") quiet = true;
    else
    {
      usage();
      return a == "--help" || a == "-h" ? 0 : 2;
    }
  }
  if(headless && script.empty())
  {
    fprintf(stderr, "--headless требует --script\n");
    return 2;
  }

  signal(SIGPIPE, SIG_IGN);
  loadSymbols(std::string(argv[0]) + ".syms");

  void* mem = mmap(nullptr, sizeof(Board), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if(mem == MAP_FAILED)
  {
    perror("mmap");
    return 1;
  }
  board = new(mem) Board();
  board->rate_x1000 = (uint32_t)(constrainD(rate, 0.05, 20) * 1000);
  board->vbat_mv = vbat;
  board->usb = usb;
  board->adc_bits = 10;

  int cmd_pipe[2], evt_pipe[2], log_pipe[2];
  if(pipe(cmd_pipe) || pipe(evt_pipe) || pipe(log_pipe))
  {
    perror("pipe");
    return 1;
  }
  // зигота порождается до запуска потоков: fork из многопоточного процесса небезопасен
  pid_t z = fork();
  if(z == 0)
  {
    close(cmd_pipe[1]);
    close(evt_pipe[0]);
    close(log_pipe[0]);
    zygoteMain(cmd_pipe[0], evt_pipe[1], log_pipe[1]);
  }
  close(cmd_pipe[0]);
  close(evt_pipe[1]);
  close(log_pipe[1]);
  zyg_cmd = cmd_pipe[1];
  zyg_evt = evt_pipe[0];

  std::thread(clockThread).detach();
  std::thread(logReaderThread, log_pipe[0]).detach();
  std::thread(phoneThread).detach();
  std::thread(supervisorThread).detach();

  if(!headless)
  {
    int lf = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    setsockopt(lf, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if(bind(lf, (sockaddr*)&addr, sizeof(addr)) || listen(lf, 16))
    {
      fprintf(stderr, "не удалось открыть порт %d: %s\n", port, strerror(errno));
      return 1;
    }
    fprintf(stderr, "\n  Эмулятор IMBA TV: http://localhost:%d\n\n", port);
    std::thread(httpThread, lf).detach();
  }

  if(!script.empty())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    scriptRun(script);
    if(headless)
    {
      int code = (script_failures || crashed_flag) ? 1 : 0;
      logf("сценарий завершён: %s", code ? "ЕСТЬ ОШИБКИ" : "без ошибок");
      killFirmware();
      fflush(stderr);
      _exit(code);
    }
  }
  for(;;) pause();
}
