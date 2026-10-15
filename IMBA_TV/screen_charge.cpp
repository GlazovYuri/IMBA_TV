#include "screen_charge.h"
#include "screen_text.h"
#include "logo.h"
#include "GyverOLED.h"
#include <math.h>

extern GyverOLED<SSH1106_128x64> oled;

static const int scr_width = 128;
static const int scr_height = 64;

struct point_t
{
  int8_t x, y;
};

// молния: верх уходит влево вниз, ступенька вправо, длинный хвост остриём вниз. Под ней проценты.
// Координаты по границам точек: многоугольник (0,0)-(1,1) закрывает одну точку
static const point_t bolt[] = {{16, 0}, {27, 0}, {19, 19}, {27, 19}, {7, 46}, {11, 26}, {2, 26}};
static const int bolt_height = 46;
static const int bolt_x = 50;               // начало координат многоугольника на экране
static const int bolt_left = bolt_x + 2;    // крайние столбцы молнии
static const int bolt_right = bolt_x + 26;
static const int bolt_mid = 64;             // середина молнии, по ней стоят проценты

// значения по бокам: крупно (scale 2), подписи мелко
static const int top_y = 4;     // напряжение и ток
static const int temp_y = 34;   // температуры
static const int label_y = 55;  // подписи под температурами
static const int pct_y = 48;    // проценты под молнией

static uint8_t style = 0;
static uint32_t open_ms = 0;

// ===== фигура молнии =====

// фигура по строкам: бит x - точка x. Контур - точки фигуры, у которых хотя бы одна соседняя
// по стороне снаружи, внутренность - остальные. Считается один раз
struct shape_t
{
  uint64_t outline[scr_height];
  uint64_t inner[scr_height];
  uint16_t area;  // точек внутри
  // обход контура по часовой стрелке от левой верхней точки
  static const int contour_max = 256;
  uint8_t cx[contour_max], cy[contour_max];
  uint16_t contour_len;
  // точки контура по углу от середины фигуры: номера в обходе и углы * 1000
  uint8_t by_angle[contour_max];
  int16_t angle[contour_max];
  float mid_x, mid_y;
};

// соседи по часовой стрелке, начиная с левого: x вправо, y вниз
static const int8_t ring_dx[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
static const int8_t ring_dy[8] = {0, -1, -1, -1, 0, 1, 1, 1};

static bool rowsGet(const uint64_t rows[scr_height], int x, int y)
{
  return x >= 0 && x < 64 && y >= 0 && y < scr_height && (rows[y] >> x & 1);
}

// обход по соседям (Moore): от левой верхней точки, слева от неё пусто
static void contourTrace(const uint64_t rows[scr_height], shape_t& shape)
{
  shape.contour_len = 0;
  int sy = 0;
  while(sy < scr_height && !rows[sy]) sy++;
  if(sy == scr_height) return;
  int sx = 0;
  while(!(rows[sy] >> sx & 1)) sx++;

  int x = sx, y = sy, back = 0;  // back - направление на пустую клетку, с которой начинается поиск
  do
  {
    shape.cx[shape.contour_len] = x;
    shape.cy[shape.contour_len] = y;
    shape.contour_len++;
    int d = -1;
    for(int i = 1; i <= 8; i++)
    {
      int k = (back + i) % 8;
      if(rowsGet(rows, x + ring_dx[k], y + ring_dy[k]))
      {
        d = k;
        break;
      }
    }
    if(d < 0) break;  // одна точка
    // пустая клетка перед найденной становится точкой возврата для нового места
    int bx = x + ring_dx[(d + 7) % 8], by = y + ring_dy[(d + 7) % 8];
    x += ring_dx[d];
    y += ring_dy[d];
    for(int k = 0; k < 8; k++)
      if(x + ring_dx[k] == bx && y + ring_dy[k] == by) back = k;
  } while((x != sx || y != sy) && shape.contour_len < shape_t::contour_max);

  int min_x = 63, max_x = 0, min_y = 63, max_y = 0;
  for(int i = 0; i < shape.contour_len; i++)
  {
    min_x = min(min_x, (int)shape.cx[i]);
    max_x = max(max_x, (int)shape.cx[i]);
    min_y = min(min_y, (int)shape.cy[i]);
    max_y = max(max_y, (int)shape.cy[i]);
  }
  shape.mid_x = (min_x + max_x) / 2.0f;
  shape.mid_y = (min_y + max_y) / 2.0f;
  for(int i = 0; i < shape.contour_len; i++)
  {
    shape.by_angle[i] = i;
    shape.angle[i] = (int16_t)lroundf(atan2f(shape.cy[i] - shape.mid_y, shape.cx[i] - shape.mid_x) * 1000);
  }
  for(int i = 1; i < shape.contour_len; i++)
    for(int j = i; j > 0 && shape.angle[j - 1] > shape.angle[j]; j--)
    {
      int16_t a = shape.angle[j];
      shape.angle[j] = shape.angle[j - 1];
      shape.angle[j - 1] = a;
      uint8_t b = shape.by_angle[j];
      shape.by_angle[j] = shape.by_angle[j - 1];
      shape.by_angle[j - 1] = b;
    }
}

// номер точки контура, которая видна из середины фигуры под углом a (радианы)
static int contourAt(const shape_t& shape, float a)
{
  int16_t key = (int16_t)lroundf(a * 1000);
  int lo = 0, hi = shape.contour_len - 1;
  while(lo < hi)
  {
    int m = (lo + hi) / 2;
    if(shape.angle[m] < key) lo = m + 1;
    else hi = m;
  }
  return shape.by_angle[lo];
}

// точка внутри, если внутри многоугольника её центр (правило чёт-нечет)
static void shapeBuild(const point_t* pts, int n, shape_t& shape)
{
  uint64_t rows[scr_height];
  for(int y = 0; y < scr_height; y++)
  {
    float yc = y + 0.5f;
    float xs[16];
    int cnt = 0;
    for(int i = 0; i < n && cnt < 16; i++)
    {
      point_t a = pts[i], b = pts[(i + 1) % n];
      if((yc > a.y) != (yc > b.y)) xs[cnt++] = a.x + (yc - a.y) * (b.x - a.x) / (b.y - a.y);
    }
    for(int i = 1; i < cnt; i++)
      for(int j = i; j > 0 && xs[j - 1] > xs[j]; j--)
      {
        float tmp = xs[j];
        xs[j] = xs[j - 1];
        xs[j - 1] = tmp;
      }
    uint64_t r = 0;
    for(int k = 0; k + 1 < cnt; k += 2)
      for(int x = 0; x < 64; x++)
      {
        float xc = x + 0.5f;
        if(xc > xs[k] && xc < xs[k + 1]) r |= 1ULL << x;
      }
    rows[y] = r;
  }

  shape.area = 0;
  for(int y = 0; y < scr_height; y++)
  {
    uint64_t r = rows[y];
    uint64_t up = y > 0 ? rows[y - 1] : 0;
    uint64_t down = y < scr_height - 1 ? rows[y + 1] : 0;
    shape.inner[y] = r & up & down & (r << 1) & (r >> 1);
    shape.outline[y] = r & ~shape.inner[y];
    shape.area += __builtin_popcountll(shape.inner[y]);
  }
  contourTrace(rows, shape);
}

static const shape_t& boltShape()
{
  static shape_t shape;
  static bool ready = false;
  if(!ready) shapeBuild(bolt, sizeof(bolt) / sizeof(bolt[0]), shape);
  ready = true;
  return shape;
}

// строка, с которой молния залита: заливка снизу занимает percent % её площади.
// По высоте узкий хвост съел бы первые проценты, и малый заряд не был бы виден
static int levelRow(const shape_t& shape, uint8_t percent)
{
  int need = ((int)shape.area * constrain(percent, 0, 100) + 50) / 100;
  int y = scr_height, sum = 0;
  while(y > 0 && sum < need) sum += __builtin_popcountll(shape.inner[--y]);
  return y;
}

static void drawBits(int y, uint64_t bits)
{
  for(int x = 0; bits; x++, bits >>= 1)
    if(bits & 1) oled.dot(bolt_x + x, y);
}

// крайние точки строки: false, если в ней ничего нет
static bool rowSpan(uint64_t bits, int& first, int& last)
{
  if(!bits) return false;
  first = 0;
  while(!(bits >> first & 1)) first++;
  last = 63;
  while(!(bits >> last & 1)) last--;
  return true;
}

// ===== значения =====

static int textWidth(const char* text)
{
  int chars = 0;
  for(const char* p = text; *p; p++) if((*p & 0xC0) != 0x80) chars++;
  return chars ? chars * 6 - 1 : 0;
}

static void smallText(int x, int y, const char* text)
{
  oled.setScale(1);
  oled.setCursorXY(x, y);
  oled.print(text);
}

// "°C": знак градуса рисуется рамкой 3×3, у шрифта его нет. В UTF-8 байт 0xB0 первым не бывает
static const char unit_degree[] = "\xb0";
static const int degree_width = 9;

static int unitWidth(const char* unit)
{
  return (unit[0] == unit_degree[0]) ? degree_width : textWidth(unit);
}

// единица мелко, по низу крупного числа
static void drawUnit(int x, int y, const char* unit)
{
  if(unit[0] != unit_degree[0])
  {
    smallText(x, y, unit);
    return;
  }
  oled.rect(x, y, x + 2, y + 2, OLED_STROKE);
  smallText(x + 4, y, "C");
}

// крупное число и единица, начиная с left
static void bigLeft(int left, int y, const char* num, const char* unit)
{
  int w = screenTextBigWidth(num);
  screenTextBig(left + w - 1, y, num);
  drawUnit(left + w + 2, y + 7, unit);
}

// то же, прижато к правому краю right
static void bigRight(int right, int y, const char* num, const char* unit)
{
  int uw = unitWidth(unit);
  drawUnit(right - uw + 1, y + 7, unit);
  screenTextBig(right - uw - 2, y, num);
}

struct values_t
{
  char voltage[6];
  char current[6];
  char battery[6];
  char charger[6];
};

// целые градусы
static void tempText(bool has, int16_t t_x10, char* text, size_t size)
{
  if(!has)
  {
    snprintf(text, size, "--");
    return;
  }
  int t = (t_x10 >= 0) ? (t_x10 + 5) / 10 : -((-t_x10 + 5) / 10);
  snprintf(text, size, "%d", constrain(t, -99, 999));
}

static void valuesText(const euc_data_t& data, const snapshot_charge_t& c, values_t& v)
{
  // напряжение крупно помещается только целыми вольтами
  if(data.present & EUC_HAS_VOLTAGE) snprintf(v.voltage, sizeof(v.voltage), "%u", (data.voltage_x10 + 5) / 10);
  else snprintf(v.voltage, sizeof(v.voltage), "--");

  // от 10 А без десятых
  if(!(c.present & SNAP_HAS_CURRENT)) snprintf(v.current, sizeof(v.current), "--");
  else if(c.current_x10 >= 100) snprintf(v.current, sizeof(v.current), "%u", (c.current_x10 + 5) / 10);
  else snprintf(v.current, sizeof(v.current), "%u.%u", c.current_x10 / 10, c.current_x10 % 10);

  tempText(c.present & SNAP_HAS_BATTERY_TEMP, c.battery_temp_x10, v.battery, sizeof(v.battery));
  tempText(c.present & SNAP_HAS_CHARGER_TEMP, c.charger_temp_x10, v.charger, sizeof(v.charger));
}

static void drawSides(const values_t& v)
{
  bigLeft(0, top_y, v.voltage, "В");
  bigRight(scr_width - 1, top_y, v.current, "А");
  bigLeft(0, temp_y, v.battery, unit_degree);
  smallText(0, label_y, "hottest");
  bigRight(scr_width - 1, temp_y, v.charger, unit_degree);
  smallText(scr_width - textWidth("charger"), label_y, "charger");
}

// проценты крупно посередине под молнией
static void drawPercent(uint8_t percent, bool known)
{
  char text[6];
  if(known) snprintf(text, sizeof(text), "%u", percent);
  else snprintf(text, sizeof(text), "--");
  int w = screenTextBigWidth(text);
  int x = bolt_mid - (w + 2 + textWidth("%") - 1) / 2;
  screenTextBig(x + w - 1, pct_y, text);
  drawUnit(x + w + 2, pct_y + 7, "%");
}

// ===== анимация =====

static float clamp01(float e)
{
  return e < 0 ? 0 : (e > 1 ? 1 : e);
}

static float easeOut(float e)
{
  float k = 1 - clamp01(e);
  return 1 - k * k * k;
}

static float easeInOut(float e)
{
  e = clamp01(e);
  return e * e * (3 - 2 * e);
}

// псевдослучайное 0..1 от координат: у каждой точки своя задержка, без списка частиц
static float hash01(int x, int y)
{
  uint32_t h = x * 374761393u + y * 668265263u;
  h = (h ^ (h >> 13)) * 1274126177u;
  h ^= h >> 16;
  return (h & 0xFFFFFF) / 16777216.0f;
}

static bool logoGet(int x, int y)
{
  return logo_bitmap[(y >> 3) * scr_width + x] >> (y & 7) & 1;
}

// значения выезжают от молнии к краям экрана: r от 0 до 1. Текст уже нарисован, ещё не открытое стирается
static void revealSides(float r)
{
  r = easeOut(r);
  if(r >= 1) return;
  int left_to = bolt_left - 1 - (int)lroundf(bolt_left * r);
  if(left_to >= 0) oled.rect(0, 0, left_to, scr_height - 1, OLED_CLEAR);
  int right_from = bolt_right + 1 + (int)lroundf((scr_width - 1 - bolt_right) * r);
  if(right_from <= scr_width - 1) oled.rect(right_from, 0, scr_width - 1, scr_height - 1, OLED_CLEAR);
}

// ----- 0: молния наливается жидкостью, волна и пузырьки -----

static const uint32_t liquid_reveal = 350, liquid_reveal_len = 700;  // значения выезжают, мс
static const uint32_t liquid_pour = 1400;  // заливка от пустой до заряда

struct bubble_t
{
  int8_t dx;        // от середины строки
  uint8_t size;     // 1 или 2 px
  uint16_t period;  // мс от дна до поверхности
  uint16_t phase;
};
static const bubble_t bubbles[] = {{-2, 2, 2300, 0}, {2, 1, 1700, 500}, {0, 2, 2900, 1400}, {-1, 1, 2000, 1100}, {3, 1, 2600, 2000}};

// возвращает заряд, который сейчас показан: при открытии он растёт вместе с заливкой
static uint8_t drawLiquid(const shape_t& shape, uint8_t charge, uint32_t t, uint32_t ms)
{
  uint8_t shown = (uint8_t)lroundf(charge * easeOut((float)t / liquid_pour));
  int level = levelRow(shape, shown);
  bool full = level == 0;
  // волна из двух синусов; пока наливается, плещется сильнее и успокаивается
  float amp = 1.0f + 2.0f * (1 - easeOut((float)t / 2200));
  int surface[64];
  for(int x = 0; x < 64; x++)
  {
    float w = 0.65f * sinf(2 * (float)M_PI * (x / 14.0f - ms / 1300.0f)) +
              0.35f * sinf(2 * (float)M_PI * (x / 6.0f + ms / 800.0f));
    surface[x] = (level >= bolt_height) ? bolt_height : (full ? 0 : level + (int)lroundf(amp * w));
  }

  for(int y = 0; y < bolt_height; y++)
  {
    uint64_t lit = shape.outline[y];
    for(int x = 0; x < 64; x++)
      if((shape.inner[y] >> x & 1) && y >= surface[x]) lit |= 1ULL << x;
    drawBits(y, lit);
  }

  // пузырьки всплывают вдоль молнии, у поверхности лопаются брызгой
  if(level >= bolt_height - 4) return shown;
  for(unsigned i = 0; i < sizeof(bubbles) / sizeof(bubbles[0]); i++)
  {
    const bubble_t& b = bubbles[i];
    float p = (float)((ms + b.phase) % b.period) / b.period;
    int bottom = bolt_height - 3, top = full ? 1 : level + 1;
    int y = bottom - (int)lroundf(p * (bottom - top));
    int first, last;
    if(!rowSpan(shape.inner[y], first, last)) continue;
    int x = (first + last) / 2 + b.dx + (int)lroundf(sinf(ms / 300.0f + i * 1.7f));
    x = constrain(x, first + 1, last - b.size);
    if(x < first + 1) continue;
    if(p > 0.92f && !full)
    {
      // брызга над поверхностью
      int sy = surface[x] - 2 - (p > 0.96f ? 1 : 0);
      if(sy >= 0 && (shape.inner[sy] >> x & 1)) oled.dot(bolt_x + x, sy, 1);
      continue;
    }
    for(int dy = 0; dy < b.size; dy++)
      for(int dx = 0; dx < b.size; dx++)
        if(y - dy >= surface[x + dx] + 1) oled.dot(bolt_x + x + dx, y - dy, 0);
  }
  return shown;
}

// ----- 1: логотип IMBA собирается в молнию, по пустой части бежит импульс, по контуру искра -----

static const uint32_t morph_hold = 500;    // логотип целиком, мс
static const uint32_t morph_spread = 350;  // разброс старта точек: первыми летят дальние от середины
static const uint32_t morph_fly = 650;     // полёт точки
static const uint32_t morph_end = morph_hold + morph_spread + morph_fly;
static const uint32_t pulse_reveal_len = 600;
static const uint32_t pulse_pour = 900;
static const uint32_t pulse_move = 700, pulse_period = 1300;  // импульс от уровня до верха и его повтор
static const uint32_t spark_lap = 2600;                       // искра обегает контур

// точки логотипа летят на контур молнии: каждая туда, куда она смотрит от середины логотипа.
// Логотип втягивается от краёв к середине. Ближе к концу контур проявляется весь, чтобы куда
// не долетели точки, тоже было видно
static void drawMorph(const shape_t& shape, uint32_t t)
{
  if(t < morph_hold)
  {
    oled.drawBitmap(logo_x, logo_y, logo_bitmap, logo_width, logo_height);
    return;
  }
  float tm = t - morph_hold;
  for(int y = 0; y < logo_height; y++)
    for(int x = 0; x < logo_width; x++)
    {
      if(!logoGet(x, y)) continue;
      float dist = sqrtf((x - 63.5f) * (x - 63.5f) + (y - 31.5f) * (y - 31.5f)) / 71.0f;
      float e = (tm - ((1 - dist) * 0.75f + hash01(x, y) * 0.25f) * morph_spread) / morph_fly;
      if(e <= 0)
      {
        oled.dot(x, y);
        continue;
      }
      int i = contourAt(shape, atan2f(y - 31.5f, x - 63.5f));
      float tx = bolt_x + shape.cx[i], ty = shape.cy[i];
      float q = easeInOut(e);
      float dx = tx - x, dy = ty - y, len = sqrtf(dx * dx + dy * dy);
      if(len < 1) len = 1;
      // небольшая дуга поперёк пути: логотип закручивается
      float s = 4 * clamp01(e) * (1 - clamp01(e)) * 5;
      oled.dot((int)lroundf(x + dx * q - dy / len * s), (int)lroundf(y + dy * q + dx / len * s));
    }
  float show = clamp01((tm - morph_spread) / (float)morph_fly);
  for(int i = 0; i < shape.contour_len; i++)
    if(hash01(shape.cx[i], shape.cy[i] + 64) < show) oled.dot(bolt_x + shape.cx[i], shape.cy[i]);
}

static uint8_t drawPulse(const shape_t& shape, uint8_t charge, uint32_t t, uint32_t ms)
{
  uint8_t shown = (uint8_t)lroundf(charge * easeOut((float)(t - morph_end) / pulse_pour));
  int level = levelRow(shape, shown);
  bool full = level == 0;

  // импульс в 2 строки бежит от уровня к верху. У полной молнии он тёмный и бежит по всей заливке
  uint32_t p = ms % pulse_period;
  int from = full ? bolt_height : level;
  int pulse = (t > morph_end + pulse_pour && p < pulse_move) ? from - 1 - (int)((from + 1) * p / pulse_move) : -10;

  for(int y = 0; y < bolt_height; y++)
  {
    uint64_t lit = shape.outline[y];
    if(y >= level) lit |= shape.inner[y];
    if(y == pulse || y == pulse + 1)
    {
      if(full) lit &= ~shape.inner[y];
      else lit |= shape.inner[y];
    }
    drawBits(y, lit);
  }

  // искра: яркая голова 3×3 и хвост; на залитой части хвост тёмный, иначе его не видно
  if(shape.contour_len == 0) return shown;
  int head = (int)((ms % spark_lap) * shape.contour_len / spark_lap);
  for(int k = 0; k < 6; k++)
  {
    int i = (head - k * 2 + shape.contour_len * 2) % shape.contour_len;
    int x = bolt_x + shape.cx[i], y = shape.cy[i];
    int r = (k == 0) ? 1 : 0;
    if(k < 3) oled.rect(x - r, y - r, x + r, y + r, OLED_FILL);
    if(k == 0) continue;
    for(int d = 0; d < 4; d++)
    {
      int nx = x + ring_dx[d * 2], ny = y + ring_dy[d * 2];
      if(rowsGet(shape.inner, nx - bolt_x, ny) && ny >= level) oled.dot(nx, ny, 0);
    }
  }
  return shown;
}

// ----- 2: молния из 10 делений: загораются по очереди, следующее мигает, по пустым бежит волна -----

static const uint32_t seg_reveal = 150, seg_reveal_len = 600;
static const uint32_t seg_start = 250;  // первое деление при открытии экрана, мс
static const uint32_t seg_step = 90;    // следующее
static const uint32_t seg_blink = 450;
static const uint32_t sweep_period = 2400, sweep_step = 70;

static uint8_t drawSegments(const shape_t& shape, uint8_t charge, uint32_t t, uint32_t ms)
{
  int full = charge / 10;
  bool intro = t < seg_start + full * seg_step + 400;
  int lit_cnt = (t < seg_start) ? 0 : min(full, (int)((t - seg_start) / seg_step) + 1);

  // деление k снизу - десятая часть площади: строки от top[k] до начала предыдущего, верхняя строка - зазор
  int top[10];
  for(int k = 0; k < 10; k++) top[k] = levelRow(shape, (k + 1) * 10);

  bool on[10];
  uint32_t sp = ms % sweep_period;
  for(int k = 0; k < 10; k++)
  {
    on[k] = k < lit_cnt;
    if(intro) continue;
    // следующее мигает
    if(k == full) on[k] = (ms / seg_blink) % 2 == 0;
    // волна: пустые деления загораются снизу вверх и гаснут разом
    if(k > full && sp < (uint32_t)(10 - full) * sweep_step + 150 && sp >= (uint32_t)(k - full) * sweep_step) on[k] = true;
    // полная молния: тёмная полоса пробегает снизу вверх
    if(full >= 10 && sp / sweep_step == (uint32_t)k) on[k] = false;
  }

  int k = 0;
  for(int y = bolt_height - 1; y >= 0; y--)
  {
    while(k < 9 && y < top[k]) k++;
    uint64_t lit = shape.outline[y];
    bool gap = (y == top[k]) && k < 9;
    if(!gap && on[k]) lit |= shape.inner[y];
    drawBits(y, lit);
  }
  // при открытии проценты идут вместе с делениями
  return intro ? min((int)charge, lit_cnt * 10) : charge;
}

// ===== экран =====

void screenChargeOpen(uint8_t s)
{
  style = s % charge_styles;
  open_ms = millis();
}

void screenChargeDraw(const euc_data_t& data, const snapshot_charge_t& charge)
{
  uint32_t ms = millis();
  uint32_t t = ms - open_ms;
  const shape_t& shape = boltShape();
  bool known = data.present & EUC_HAS_CHARGE;
  uint8_t percent = known ? constrain(data.charge, 0, 100) : 0;

  if(style == 1 && t < morph_end)
  {
    drawMorph(shape, t);
    return;
  }

  values_t v;
  valuesText(data, charge, v);
  // текст ближе 8 px по высоте: побайтовая запись стёрла бы соседний
  oled.textMode(BUF_ADD);
  drawSides(v);
  uint8_t shown;
  if(style == 0)
  {
    revealSides(((int32_t)t - (int32_t)liquid_reveal) / (float)liquid_reveal_len);
    shown = drawLiquid(shape, percent, t, ms);
  }
  else if(style == 1)
  {
    revealSides(((int32_t)t - (int32_t)morph_end) / (float)pulse_reveal_len);
    shown = drawPulse(shape, percent, t, ms);
  }
  else
  {
    revealSides(((int32_t)t - (int32_t)seg_reveal) / (float)seg_reveal_len);
    shown = drawSegments(shape, percent, t, ms);
  }
  drawPercent(shown, known);
  oled.textMode(BUF_REPLACE);
}
