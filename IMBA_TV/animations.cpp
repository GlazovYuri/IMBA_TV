#include "animations.h"
#include "display.h"
#include "config.h"
#include "utils.h"
#include "GyverOLED.h"
#include "logo.h"
#include <math.h>
#include <nrf_soc.h>
#include "intro_data.h"

extern GyverOLED<SSH1106_128x64> oled;

static void fillRect(int x0, int y0, int x1, int y1, uint8_t fill)
{
  for(int y = y0; y <= y1; y++)
    for(int x = x0; x <= x1; x++)
      oled.dot(x, y, fill);
}

// ===== Выключение: схлопывание в линию (ЭЛТ) =====
void displayPowerOffAnimation(uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  static const uint32_t collapse_time = 200; //ms
  static const uint32_t shrink_time = 150; //ms
  static const uint32_t fade_time = 100; //ms
  static const int cx = 64;
  static const int cy = 32;

  oled.invertDisplay(false);
  if(!config().poweroff_animation)
  {
    oled.clear();
    oled.update();
    return;
  }

  euc_data_t snapshot = data;
  uint8_t contrast = displayGetContrast();
  uint32_t start = millis();
  uint32_t t;
  while((t = millis() - start) < collapse_time + shrink_time + fade_time)
  {
    if(t < collapse_time)
    {
      float k = 1.0f - (float)t / collapse_time;
      int half = 1 + (int)(31.0f * k * k * k);
      displayRenderIface(dev_charge, is_charging, snapshot);
      fillRect(0, 0, 127, cy - half - 1, 0);
      fillRect(0, cy + half, 127, 63, 0);
      fillRect(0, cy - half, 127, cy - half, 1);
      fillRect(0, cy + half - 1, 127, cy + half - 1, 1);
    }
    else if(t < collapse_time + shrink_time)
    {
      float k = 1.0f - (float)(t - collapse_time) / shrink_time;
      int half = 1 + (int)(63.0f * k * k);
      oled.clear();
      fillRect(cx - half, cy - 1, cx + half - 1, cy, 1);
    }
    else
    {
      float k = 1.0f - (float)(t - collapse_time - shrink_time) / fade_time;
      oled.setContrast((uint8_t)(contrast * k));
      oled.clear();
      fillRect(cx - 1, cy - 1, cx, cy, 1);
    }
    oled.update();
  }

  oled.clear();
  oled.update();
}

//   0 - волна с пузырями, 1 - разрушение блока, 2 - взрыв TNT (Minecraft),
//   3..5 - Clawd: печатает интерфейс, экран ноутбука, пересборка из пикселей.

static const uint32_t intro_logo_hold = 1000; //ms
static const int scr_w = 128;
static const int scr_h = 64;
static const int buf_size = scr_w * scr_h / 8;

// все буферы в формате drawBitmap: байт = 8 пикселей по вертикали, страницы по строкам
static uint8_t mask_clear[buf_size];  // что стереть из интерфейса
static uint8_t mask_add[buf_size];    // что дорисовать (логотип, кромка волны)
static uint8_t layer_black[buf_size]; // слой эффектов: чёрные рамки
static uint8_t layer_white[buf_size]; // слой эффектов: белые пиксели

static uint32_t rnd_state = 12345;

static float rnd()
{
  rnd_state = rnd_state * 1664525u + 1013904223u;
  return (rnd_state >> 8) / 16777216.0f;
}

static inline void bufSet(uint8_t* b, int x, int y) { b[(y >> 3) * scr_w + x] |= (1 << (y & 7)); }
static inline void bufClr(uint8_t* b, int x, int y) { b[(y >> 3) * scr_w + x] &= ~(1 << (y & 7)); }
static inline bool bufGet(const uint8_t* b, int x, int y) { return b[(y >> 3) * scr_w + x] & (1 << (y & 7)); }

static void layersClear()
{
  memset(layer_black, 0, buf_size);
  memset(layer_white, 0, buf_size);
}

static void layerWhite(int x, int y)
{
  if((unsigned)x >= (unsigned)scr_w || (unsigned)y >= (unsigned)scr_h) return;
  bufSet(layer_white, x, y);
  bufClr(layer_black, x, y);
}

static void layerBlack(int x, int y)
{
  if((unsigned)x >= (unsigned)scr_w || (unsigned)y >= (unsigned)scr_h) return;
  bufSet(layer_black, x, y);
  bufClr(layer_white, x, y);
}

static void introFrame(uint8_t dev_charge, bool is_charging, euc_data_t& data, bool masks, bool layers)
{
  displayRenderIface(dev_charge, is_charging, data);
  if(masks)
  {
    oled.drawBitmap(0, 0, mask_clear, scr_w, scr_h, BITMAP_NORMAL, BUF_SUBTRACT);
    oled.drawBitmap(0, 0, mask_add, scr_w, scr_h, BITMAP_NORMAL, BUF_ADD);
  }
  if(layers)
  {
    oled.drawBitmap(0, 0, layer_black, scr_w, scr_h, BITMAP_NORMAL, BUF_SUBTRACT);
    oled.drawBitmap(0, 0, layer_white, scr_w, scr_h, BITMAP_NORMAL, BUF_ADD);
  }
  oled.update();
}

static uint32_t introWait(uint32_t start, uint32_t step)
{
  uint32_t elapsed = millis() - start;
  if(elapsed < step) delay(step - elapsed);
  return millis();
}

// ===== 0. Волна с пузырями =====

static const uint32_t wave_step = 40; //ms
static const int wave_sweep_steps = 28;
static const int wave_tail_steps = 18;
static const int bubble_cols = 9;
static const int bubble_rows = 4;
static const int bubble_cnt = bubble_cols * bubble_rows;

struct bubble_t
{
  float x, y, drift, rise, phase;
  uint8_t r, life;
  int8_t born;
};

static bubble_t bubbles[bubble_cnt];

static float waveEdge(float y, int t)
{
  float p = (t < wave_sweep_steps) ? (float)t / wave_sweep_steps : 1.0f;
  p = p * p * (3.0f - 2.0f * p);
  float base = -14.0f + p * (scr_w + 28);
  return base + 6.0f * sinf(y * 0.2f + t * 0.5f) + 1.5f * sinf(y * 0.45f - t * 0.8f);
}

static void waveInit()
{
  int i = 0;
  for(int c = 0; c < bubble_cols; c++)
  {
    for(int r = 0; r < bubble_rows; r++)
    {
      bubble_t& b = bubbles[i++];
      b.x = (c + 0.15f + 0.7f * rnd()) * scr_w / bubble_cols;
      b.y = (r + 0.3f + 0.7f * rnd()) * scr_h / bubble_rows;

      int born = 0;
      float best = 1e9f;
      for(int t = 0; t <= wave_sweep_steps; t++)
      {
        float d = fabsf(waveEdge(b.y, t) - b.x);
        if(d < best) { best = d; born = t; }
      }
      b.born = born + (int)(rnd() * 4);
      b.life = 10 + (int)(rnd() * 9);
      b.r = 3 + (int)(rnd() * 5);
      b.drift = (rnd() - 0.5f) * 0.5f;
      b.rise = 0.7f + rnd() * 0.7f;
      b.phase = rnd() * 6.28f;
    }
  }
}

static void waveBuildMasks(int t)
{
  memset(mask_clear, 0, buf_size);
  memset(mask_add, 0, buf_size);
  bool edge_on = (t <= wave_sweep_steps);

  for(int y = 0; y < scr_h; y++)
  {
    float ex = waveEdge(y, t);
    int x0 = (int)floorf(ex - 1.3f);
    if(x0 < 0) x0 = 0;
    uint8_t bit = 1 << (y & 7);

    for(int x = x0; x < scr_w; x++)
    {
      float dx = x - ex;
      int idx = (y >> 3) * scr_w + x;

      if(edge_on && fabsf(dx) < 1.3f)
      {
        mask_clear[idx] |= bit;
        mask_add[idx] |= bit;
      }
      else if(edge_on && dx >= 1.0f && dx < 2.6f)
      {
        mask_clear[idx] |= bit;
      }
      else if(dx >= 0.0f)
      {
        mask_clear[idx] |= bit;
        mask_add[idx] |= logo_bitmap[idx] & bit;
      }
    }
  }
}

static int ring_x[96];
static int ring_y[96];
static int ring_cnt;

static void ringAdd(int x, int y)
{
  if(ring_cnt < 96)
  {
    ring_x[ring_cnt] = x;
    ring_y[ring_cnt] = y;
    ring_cnt++;
  }
}

static void bubblePoints(const bubble_t& b, int t)
{
  ring_cnt = 0;
  int age = t - b.born;
  if(age < 0 || age >= b.life) return;

  float cx = b.x + b.drift * age + 1.2f * sinf(age * 0.6f + b.phase);
  float cy = b.y - b.rise * age;
  float r = b.r + age * 0.08f;

  if(age >= b.life - 2)
  {
    int k = age - (b.life - 3);
    for(int a = 0; a < 360; a += 60)
    {
      float rad = a * 0.0174533f;
      ringAdd((int)lroundf(cx + (r + k) * cosf(rad)), (int)lroundf(cy + (r + k) * sinf(rad)));
    }
    return;
  }

  int icx = (int)lroundf(cx);
  int icy = (int)lroundf(cy);
  int ir = (int)lroundf(r);

  int x = ir, y = 0, err = 1 - ir;
  while(x >= y)
  {
    ringAdd(icx + x, icy + y); ringAdd(icx - x, icy + y);
    ringAdd(icx + x, icy - y); ringAdd(icx - x, icy - y);
    ringAdd(icx + y, icy + x); ringAdd(icx - y, icy + x);
    ringAdd(icx + y, icy - x); ringAdd(icx - y, icy - x);
    y++;
    if(err < 0) err += 2 * y + 1;
    else { x--; err += 2 * (y - x) + 1; }
  }

  if(ir >= 3) ringAdd((int)lroundf(cx - r * 0.45f), (int)lroundf(cy - r * 0.45f));
}

static void waveDrawBubbles(int t)
{
  layersClear();
  for(int i = 0; i < bubble_cnt; i++)
  {
    bubblePoints(bubbles[i], t);
    for(int p = 0; p < ring_cnt; p++)
      for(int dy = -1; dy <= 1; dy++)
        for(int dx = -1; dx <= 1; dx++)
          layerBlack(ring_x[p] + dx, ring_y[p] + dy);
  }
  for(int i = 0; i < bubble_cnt; i++)
  {
    bubblePoints(bubbles[i], t);
    for(int p = 0; p < ring_cnt; p++)
      layerWhite(ring_x[p], ring_y[p]);
  }
}

static void introWave(uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  waveInit();
  uint32_t start = millis();
  for(int t = 0; t < wave_sweep_steps + wave_tail_steps; t++)
  {
    waveBuildMasks(t);
    waveDrawBubbles(t);
    introFrame(dev_charge, is_charging, data, true, true);
    start = introWait(start, wave_step);
  }
}

// ===== 1. Разрушение блока =====

static const uint32_t break_step = 40; //ms
static const int break_steps_per_stage = 2;
static const int break_fall_steps = 28;
static const float break_gravity = 0.32f;
static const int tile_size = 8;
static const int tile_max = (scr_w / tile_size) * (scr_h / tile_size);

struct tile_t
{
  float x0, y0, vx, vy;
  uint8_t tx, ty, life;
};

static tile_t tiles[tile_max];
static int tile_cnt;
static const uint8_t tile_ones[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static bool crackPixel(int stage, int x, int y)
{
  int rot = ((x >> 5) + 2 * (y >> 5)) & 3;
  int lx = x & 31;
  int ly = y & 31;
  int row, col;
  switch(rot)
  {
    case 0:  row = ly;      col = lx;      break;
    case 1:  row = lx;      col = 31 - ly; break;
    case 2:  row = 31 - ly; col = 31 - lx; break;
    default: row = 31 - lx; col = ly;      break;
  }
  return (mc_crack[stage][row >> 1] >> (col >> 1)) & 1;
}

static void breakInit()
{
  tile_cnt = 0;
  for(int ty = 0; ty < scr_h / tile_size; ty++)
  {
    for(int tx = 0; tx < scr_w / tile_size; tx++)
    {
      const uint8_t* src = &logo_bitmap[ty * scr_w + tx * tile_size];
      bool any = false;
      for(int i = 0; i < tile_size; i++) any |= (src[i] != 0);
      if(!any) continue;

      tile_t& t = tiles[tile_cnt++];
      float cx = tx * tile_size + tile_size / 2.0f;
      t.tx = tx;
      t.ty = ty;
      t.x0 = tx * tile_size;
      t.y0 = ty * tile_size;
      t.vx = (cx - scr_w / 2.0f) / (scr_w / 2.0f) * 2.0f + (rnd() * 1.2f - 0.6f);
      t.vy = -(1.0f + rnd() * 2.2f);
      t.life = 12 + (int)(rnd() * 17);
    }
  }
}

static void introBreak(uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  breakInit();
  uint32_t start = millis();

  for(int stage = 0; stage < 10; stage++)
  {
    for(int s = 0; s < break_steps_per_stage; s++)
    {
      if(s == 0)
      {
        memcpy(mask_add, logo_bitmap, buf_size);
        for(int y = 0; y < scr_h; y++)
          for(int x = 0; x < scr_w; x++)
            if(crackPixel(stage, x, y)) bufClr(mask_add, x, y);
      }
      oled.drawBitmap(0, 0, mask_add, scr_w, scr_h, BITMAP_NORMAL, BUF_REPLACE);
      oled.update();
      start = introWait(start, break_step);
    }
  }

  for(int t = 0; t < break_fall_steps; t++)
  {
    displayRenderIface(dev_charge, is_charging, data);
    for(int i = 0; i < tile_cnt; i++)
    {
      const tile_t& tl = tiles[i];
      if(t >= tl.life) continue;

      float x = tl.x0 + tl.vx * t;
      float y = tl.y0 + tl.vy * t + 0.5f * break_gravity * t * t;
      if(y > scr_h || x < -tile_size || x > scr_w) continue;

      int px = (int)lroundf(x);
      int py = (int)lroundf(y);
      oled.drawBitmap(px, py, tile_ones, tile_size, tile_size, BITMAP_NORMAL, BUF_SUBTRACT);
      oled.drawBitmap(px, py, &logo_bitmap[tl.ty * scr_w + tl.tx * tile_size],
                      tile_size, tile_size, BITMAP_NORMAL, BUF_ADD);
    }
    oled.update();
    start = introWait(start, break_step);
  }
}

// ===== 2. Взрыв TNT =====

static const uint32_t expl_step = 50; //ms
static const int expl_ticks = 44;
static const int puff_cnt = 170;
static const float expl_size = 4.0f;
static const float expl_px_per_block = 14.0f;
static const float expl_friction = 0.9f;
static const float expl_rise = 0.004f;
static const int expl_max_age = 40;
static const int expl_max_size = 20;
static const int expl_reveal = 16;

struct puff_t
{
  float x, y, vx, vy, z;
  uint8_t size, max_age;
};

static puff_t puffs[puff_cnt];

static void explInit()
{
  int n = 0;
  while(n < puff_cnt)
  {
    float x = (rnd() * 2.0f - 1.0f) * expl_size;
    float y = (rnd() * 2.0f - 1.0f) * expl_size;
    float z = (rnd() * 2.0f - 1.0f) * expl_size;
    float d = sqrtf(x * x + y * y + z * z);
    if(d > expl_size || d < 1e-3f) continue;

    float sp = 0.5f / (d / expl_size + 0.1f);
    float r1 = rnd();
    sp *= (r1 * rnd() + 0.3f);
    float jx = (rnd() * 2.0f - 1.0f) * 0.05f;
    float jy = (rnd() * 2.0f - 1.0f) * 0.05f;
    int age = (int)(16.0f / (rnd() * 0.8f + 0.2f)) + 2;
    float scale = rnd();
    scale = scale * rnd() * 6.0f + 1.0f;

    puff_t& p = puffs[n++];
    p.x = x / 2.0f * expl_px_per_block;
    p.y = y / 2.0f * expl_px_per_block;
    p.z = z / 2.0f;
    p.vx = (x / d * sp + jx) * expl_px_per_block;
    p.vy = (y / d * sp + jy) * expl_px_per_block;
    p.max_age = age > expl_max_age ? expl_max_age : age;
    int size = (int)lroundf(0.2f * scale * expl_px_per_block);
    p.size = size < 2 ? 2 : (size > expl_max_size ? expl_max_size : size);
  }

  for(int i = 1; i < puff_cnt; i++)
  {
    puff_t key = puffs[i];
    int j = i - 1;
    while(j >= 0 && puffs[j].z > key.z) { puffs[j + 1] = puffs[j]; j--; }
    puffs[j + 1] = key;
  }
}

static void explDrawPuff(const puff_t& p, int age)
{
  int k = 7 - age * 8 / p.max_age;
  if(k < 0) return;

  int size = p.size;
  int rows = size + 2;
  uint32_t white[expl_max_size + 2];
  uint32_t halo[expl_max_size + 2];

  for(int r = 0; r < rows; r++) white[r] = 0;
  for(int yy = 0; yy < size; yy++)
  {
    int sy = ((2 * yy + 1) * 4) / size;
    for(int xx = 0; xx < size; xx++)
    {
      int sx = ((2 * xx + 1) * 4) / size;
      if((mc_puff[k][sy] >> sx) & 1) white[yy + 1] |= (1UL << (xx + 1));
    }
  }

  uint32_t h[expl_max_size + 2];
  for(int r = 0; r < rows; r++) h[r] = white[r] | (white[r] << 1) | (white[r] >> 1);
  for(int r = 0; r < rows; r++)
    halo[r] = h[r] | (r > 0 ? h[r - 1] : 0) | (r < rows - 1 ? h[r + 1] : 0);

  int x0 = (int)lroundf(scr_w / 2.0f + p.x - size / 2.0f);
  int y0 = (int)lroundf(scr_h / 2.0f + p.y - size / 2.0f);

  for(int r = 0; r < rows; r++)
  {
    int Y = y0 - 1 + r;
    if(Y < 0 || Y >= scr_h) continue;
    for(int c = 0; c < rows; c++)
    {
      int X = x0 - 1 + c;
      if(X < 0 || X >= scr_w) continue;
      if((white[r] >> c) & 1) layerWhite(X, Y);
      else if((halo[r] >> c) & 1) layerBlack(X, Y);
    }
  }
}

static void introExplosion(uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  explInit();
  uint32_t start = millis();

  for(int t = 0; t < expl_ticks; t++)
  {
    int reveal = expl_reveal * (t + 1);
    bool all_revealed = (reveal * reveal >= (scr_w / 2) * (scr_w / 2) + (scr_h / 2) * (scr_h / 2));
    if(!all_revealed)
    {
      memset(mask_clear, 0, buf_size);
      memset(mask_add, 0, buf_size);
      int r2 = reveal * reveal;
      for(int y = 0; y < scr_h; y++)
      {
        int dy = y - scr_h / 2;
        for(int x = 0; x < scr_w; x++)
        {
          int dx = x - scr_w / 2;
          if(dx * dx + dy * dy < r2) continue;
          bufSet(mask_clear, x, y);
          if(bufGet(logo_bitmap, x, y)) bufSet(mask_add, x, y);
        }
      }
    }

    layersClear();
    for(int i = 0; i < puff_cnt; i++)
    {
      puff_t& p = puffs[i];
      if(t >= p.max_age) continue;
      explDrawPuff(p, t);
      p.x += p.vx;
      p.y += p.vy;
      p.vx *= expl_friction;
      p.vy = p.vy * expl_friction - expl_rise * expl_px_per_block;
    }

    introFrame(dev_charge, is_charging, data, !all_revealed, true);
    start = introWait(start, expl_step);
  }
}

// ===== Clawd (маскот Claude Code) =====
// Общее для трёх сценариев: каждый кадр рисуется живой интерфейс и копируется в mask_add,
// кадр собирается в layer_white и выводится целиком. Clawd 32x20: тело 24x16, глаза 2x4,
// руки 4x4 по бокам, четыре ноги 2x4. Ноутбук стоит крышкой к нему, видна её задняя сторона.

static const uint32_t clawd_step = 40; //ms

enum { EYES_OPEN, EYES_SHUT, EYES_WINK, EYES_HAPPY, EYES_BACK };

struct pose_t
{
  int8_t arm_l = 0, arm_r = 0; // насколько поднята рука, px
  int8_t leg = -1;             // какая пара ног поднята при ходьбе, -1 - стоит
  int8_t look = 0;             // сдвиг глаз по направлению взгляда
  uint8_t eyes = EYES_OPEN;
};

static const uint8_t clawd_star[5] = {0x14, 0x08, 0x3e, 0x08, 0x14}; // '*' на крышке, строки 1..5

static inline void bufPut(uint8_t* b, int x, int y, bool v)
{
  if((unsigned)x >= (unsigned)scr_w || (unsigned)y >= (unsigned)scr_h) return;
  if(v) bufSet(b, x, y);
  else bufClr(b, x, y);
}

static void bufRect(uint8_t* b, int x, int y, int w, int h, bool v)
{
  int x0 = max(x, 0), y0 = max(y, 0);
  int x1 = min(x + w, scr_w), y1 = min(y + h, scr_h);
  for(int yy = y0; yy < y1; yy++)
    for(int xx = x0; xx < x1; xx++)
      if(v) bufSet(b, xx, yy);
      else bufClr(b, xx, yy);
}

// инвертировать строки экрана выше sweep (выделение, как Ctrl+A)
static void bufInvertRows(uint8_t* b, int sweep)
{
  for(int p = 0; p < scr_h / 8 && sweep > p * 8; p++)
  {
    uint8_t m = (sweep >= p * 8 + 8) ? 0xFF : (1 << (sweep - p * 8)) - 1;
    for(int x = 0; x < scr_w; x++) b[p * scr_w + x] ^= m;
  }
}

static void clawdGrabIface(uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  displayRenderIface(dev_charge, is_charging, data);
  for(int p = 0; p < scr_h / 8; p++)
    for(int x = 0; x < scr_w; x++)
      mask_add[p * scr_w + x] = oled._oled_buffer[(x << 3) + p];
}

static void clawdShow(const uint8_t* fb)
{
  oled.drawBitmap(0, 0, fb, scr_w, scr_h, BITMAP_NORMAL, BUF_REPLACE);
  oled.update();
}

static void clawdDraw(uint8_t* b, int x, int y, const pose_t& p)
{
  int8_t parts[7][4] = {
    {4, 0, 24, 16},
    {0, (int8_t)(8 - p.arm_l), 4, 4}, {28, (int8_t)(8 - p.arm_r), 4, 4},
    {6, 16, 2, 4}, {10, 16, 2, 4}, {20, 16, 2, 4}, {24, 16, 2, 4},
  };
  for(int i = 0; i < 4; i++)
    if(p.leg >= 0 && i % 2 == p.leg) parts[3 + i][3] = 2;

  // чёрная кромка, чтобы Clawd читался поверх логотипа и интерфейса
  for(auto& r : parts) bufRect(b, x + r[0] - 1, y + r[1] - 1, r[2] + 2, r[3] + 2, false);
  for(auto& r : parts) bufRect(b, x + r[0], y + r[1], r[2], r[3], true);
  if(p.eyes == EYES_BACK) return;

  for(int i = 0; i < 2; i++)
  {
    int ex = x + (i ? 22 : 8) + p.look;
    if(p.eyes == EYES_HAPPY)
    {
      bufPut(b, ex - 1, y + 6, false);
      bufPut(b, ex, y + 5, false);
      bufPut(b, ex + 1, y + 5, false);
      bufPut(b, ex + 2, y + 6, false);
    }
    else if(p.eyes == EYES_SHUT || (p.eyes == EYES_WINK && i == 1)) bufRect(b, ex - 1, y + 6, 4, 1, false);
    else bufRect(b, ex, y + 4, 2, 4, false);
  }
}

// ноутбук перед Clawd: крышка высотой lid (0 - закрыт) и край корпуса под ней
static void clawdLaptop(uint8_t* b, int x, int y, int lid)
{
  const int lx = x + 6, lw = 20, bot = y + 18;
  bufRect(b, x + 1, bot - 1, 30, 4, false);
  if(lid > 0)
  {
    int top = bot - lid;
    bufRect(b, lx - 1, top - 1, lw + 2, lid + 2, false);
    bufRect(b, lx, top, lw, lid, true);
    if(lid > 2) bufRect(b, lx + 1, top + 1, lw - 2, lid - 2, false);
    if(lid >= 9)
    {
      int sy = top + (lid - 5) / 2;
      for(int c = 0; c < 5; c++)
        for(int r = 1; r < 6; r++)
          if((clawd_star[c] >> r) & 1) bufPut(b, lx + 8 + c, sy + r - 1, true);
    }
  }
  bufRect(b, x + 2, bot, 28, 2, true);
}

static void poseTyping(pose_t& p, int t, int period)
{
  bool k = (t / period) & 1;
  p.arm_l = k ? 2 : 0;
  p.arm_r = k ? 0 : 2;
}

// шаг: возвращает подскок тела
static int poseWalk(pose_t& p, int t)
{
  p.leg = (t / 80) & 1;
  return p.leg ? -1 : 0;
}

static int jump(int t, int len, int height)
{
  return (int)lroundf(sinf(PI * t / len) * height);
}

static float easeSmooth(float e)
{
  e = constrain(e, 0.0f, 1.0f);
  return e * e * (3.0f - 2.0f * e);
}

static float easeOut(float e)
{
  e = 1.0f - constrain(e, 0.0f, 1.0f);
  return 1.0f - e * e * e;
}

// ===== 3. Clawd печатает интерфейс =====
// Строки интерфейса ищутся в каждом кадре заново, так что набор идёт по тому, что сейчас на экране

static const int type_start = 1250;  //ms
static const int type_window = 1000; //ms на весь набор
static const int type_pause = 100;   //ms между строками
static const int type_band_max = 8;

struct band_t
{
  uint8_t y0, y1, x0, x1, cw, n;
};

static int typeBands(const uint8_t* ui, band_t* out)
{
  int cnt = 0;
  for(int y = 0; y < scr_h; y++)
  {
    int mn = scr_w, mx = -1;
    for(int x = 0; x < scr_w; x++)
      if(bufGet(ui, x, y)) { if(mn == scr_w) mn = x; mx = x; }
    if(mx < 0) continue;

    if(cnt && out[cnt - 1].y1 == y - 1)
    {
      band_t& b = out[cnt - 1];
      b.y1 = y;
      b.x0 = min((int)b.x0, mn);
      b.x1 = max((int)b.x1, mx);
    }
    else if(cnt < type_band_max)
    {
      out[cnt++] = {(uint8_t)y, (uint8_t)y, (uint8_t)mn, (uint8_t)mx, 0, 0};
    }
  }
  for(int i = 0; i < cnt; i++)
  {
    band_t& b = out[i];
    b.cw = (b.y1 - b.y0 + 1 > 9) ? 12 : 6;
    b.n = (b.x1 - b.x0 + b.cw) / b.cw;
  }
  return cnt;
}

static void typeFrame(int t, uint8_t* fb, const uint8_t* ui)
{
  static const int X0 = 92, Y = 44;

  if(t < 850)
  {
    memcpy(fb, logo_bitmap, buf_size);
  }
  else if(t < 1150)
  {
    memcpy(fb, logo_bitmap, buf_size);
    bufInvertRows(fb, (t - 850) * scr_h / 300);
  }
  else if(t >= type_start + type_window)
  {
    memcpy(fb, ui, buf_size);
  }
  else
  {
    memset(fb, 0, buf_size);
    band_t bands[type_band_max];
    int cnt = typeBands(ui, bands);
    int chars = 0;
    for(int i = 0; i < cnt; i++) chars += bands[i].n;
    int char_ms = chars ? min(60, (type_window - type_pause * cnt) / chars) : 60;

    int tt = type_start, cur = -1, cur_x = 0;
    for(int i = 0; i < cnt; i++)
    {
      const band_t& b = bands[i];
      int k = (t < tt) ? 0 : min((int)b.n, (t - tt) / char_ms);
      int lim = b.x0 + k * b.cw;
      for(int y = b.y0; y <= b.y1; y++)
        for(int x = b.x0; x < lim && x < scr_w; x++)
          if(bufGet(ui, x, y)) bufSet(fb, x, y);
      if(t >= tt - type_pause && t < tt + b.n * char_ms) { cur = i; cur_x = lim; }
      tt += b.n * char_ms + type_pause;
    }
    if(t < type_start && cnt) { cur = 0; cur_x = bands[0].x0; }
    if(cur >= 0 && ((t / 200) % 2 == 0 || t >= type_start))
      bufRect(fb, cur_x, bands[cur].y0 - 1, bands[cur].cw - 1, bands[cur].y1 - bands[cur].y0 + 3, true);
  }

  pose_t p;
  int x = X0, y = Y, lid = 10;
  if(t < 700) { x = 128 + (X0 - 128) * t / 700; y += poseWalk(p, t); p.look = -1; lid = 0; }
  else if(t < 850) lid = (t - 700) * 10 / 150;
  else if(t < 2350) poseTyping(p, t, 60);
  else if(t < 2600)
  {
    p.eyes = EYES_HAPPY;
    if(t < 2450) y -= jump(t - 2350, 100, 4);
    else lid = 10 - (t - 2450) * 10 / 150;
  }
  else { x = X0 + (136 - X0) * (t - 2600) / 600; y += poseWalk(p, t); p.look = 1; lid = 0; }

  clawdDraw(fb, x, y, p);
  clawdLaptop(fb, x, y, lid);
}

// ===== 4. Clawd: экран ноутбука =====
// Камера отъезжает: логотип был на экране ноутбука. Clawd сидит спиной, пишет код,
// по Enter код становится интерфейсом, Clawd оборачивается, камера въезжает в экран.
// Экран ноутбука при k = 1 - окно 64x32 в (32, 4), при k = 2 - весь дисплей.

// строки кода: отступ и длины слов в знаках, знак 4x2 px
static const uint8_t code_lines[][4] = {
  {0, 5, 1, 7}, {2, 4, 9, 2}, {4, 6, 5, 0}, {4, 3, 8, 4},
  {2, 2, 0, 0}, {2, 5, 6, 3}, {4, 7, 4, 0}, {0, 1, 0, 0},
};
static const int code_line_cnt = sizeof(code_lines) / sizeof(code_lines[0]);

static int codeTotal()
{
  int n = 0;
  for(int i = 0; i < code_line_cnt; i++)
    for(int j = 1; j < 4 && code_lines[i][j]; j++) n += code_lines[i][j] + 1;
  return n;
}

// n знаков кода в буфер b; курсор после последнего знака
static void codeDraw(uint8_t* b, int n, int& cx, int& cy)
{
  memset(b, 0, buf_size);
  cx = 4; cy = 4;
  for(int i = 0; i < code_line_cnt && n > 0; i++)
  {
    int x = 4 + code_lines[i][0] * 8;
    cy = 4 + i * 7;
    for(int j = 1; j < 4 && code_lines[i][j]; j++)
    {
      for(int c = 0; c < code_lines[i][j] && n > 0; c++, n--, x += 4) bufRect(b, x, cy, 4, 2, true);
      if(n > 0) { n--; x += 4; }
      cx = x;
    }
  }
}

static bool sampleOr(const uint8_t* src, float u0, float u1, float v0, float v1)
{
  int a = (int)u0, b = max(a + 1, (int)u1);
  int c = (int)v0, d = max(c + 1, (int)v1);
  for(int y = c; y < d && y < scr_h; y++)
    for(int x = a; x < b && x < scr_w; x++)
      if(bufGet(src, x, y)) return true;
  return false;
}

static void laptopFrame(int t, uint8_t* fb, const uint8_t* ui)
{
  static const int total = codeTotal();
  uint8_t* sc = mask_clear;   // ноутбук и Clawd в масштабе k = 1
  uint8_t* code = layer_black;

  float k = 1.0f;
  if(t < 600) k = 2.0f - easeSmooth(t / 600.0f);
  else if(t >= 2150) k = 1.0f + easeSmooth((t - 2150) / 600.0f);

  const uint8_t* scr = ui;
  if(t < 850)
  {
    scr = logo_bitmap;
  }
  else if(t < 1050)
  {
    memcpy(code, logo_bitmap, buf_size);
    bufInvertRows(code, (t - 850) * scr_h / 200);
    scr = code;
  }
  else if(t < 1850)
  {
    int cx, cy;
    codeDraw(code, (t < 1750) ? (t - 1050) * total / 700 : total, cx, cy);
    if(t < 1750 && (t / 120) % 2 == 0) bufRect(code, cx, cy - 2, 4, 6, true);
    if(t >= 1750)
    {
      // Enter: интерфейс сверху вниз заменяет код
      int rows = (t - 1750) * scr_h / 100;
      for(int y = 0; y < rows; y++)
        for(int x = 0; x < scr_w; x++) bufPut(code, x, y, bufGet(ui, x, y));
    }
    scr = code;
  }

  memset(sc, 0, buf_size);
  bufRect(sc, 31, 3, 66, 34, true);
  bufRect(sc, 32, 4, 64, 32, false);
  bufRect(sc, 30, 38, 68, 1, true);
  bufRect(sc, 28, 39, 72, 1, true);
  bufRect(sc, 26, 40, 76, 1, true);
  if(t >= 600)
  {
    pose_t p;
    p.eyes = EYES_BACK;
    int y = 40;
    if(t < 850) y = 64 - (int)lroundf(24 * easeOut((t - 600) / 250.0f));
    else if(t < 1750) poseTyping(p, t, 60);
    else if(t < 1850) y -= jump(t - 1750, 100, 3);
    else p.eyes = (t >= 2000 && t < 2150) ? EYES_WINK : EYES_OPEN;
    clawdDraw(sc, 48, y, p);
  }

  // приближение вокруг экрана ноутбука: точка сцены (sx, sy) уходит в (64 + (sx-64)k, cy + (sy-20)k)
  float cy = 20.0f + (k - 1.0f) * 12.0f;
  memset(fb, 0, buf_size);
  for(int y = 0; y < scr_h; y++)
  {
    int sy = (int)floorf(20.0f + (y - cy) / k);
    if(sy < 0 || sy >= scr_h) continue;
    for(int x = 0; x < scr_w; x++)
      if(bufGet(sc, (int)floorf(64.0f + (x - 64) / k), sy)) bufSet(fb, x, y);
  }

  float w = 64.0f * k, h = 32.0f * k, x0 = 64.0f - 32.0f * k, y0 = cy - 16.0f * k;
  float du = scr_w / w, dv = scr_h / h;
  for(int y = max(0, (int)ceilf(y0)); y < min((float)scr_h, y0 + h); y++)
  {
    float v = (y - y0) * dv;
    for(int x = max(0, (int)ceilf(x0)); x < min((float)scr_w, x0 + w); x++)
    {
      float u = (x - x0) * du;
      bufPut(fb, x, y, sampleOr(scr, u, u + du, v, v + dv));
    }
  }
}

// ===== 5. Clawd: пересборка из пикселей =====
// Ноутбук втягивает логотип, потом выпускает пиксели интерфейса на их места.
// Задержка и траектория пикселя считаются из его координат, поэтому изменившиеся данные
// интерфейса подхватываются сами, без списка частиц.

static const int pix_tx = 64, pix_ty = 57; // середина крышки ноутбука
static const int pix_suck_start = 750, pix_suck_fly = 300;
static const int pix_emit_start = 1550, pix_emit_fly = 380;

static float hash01(int x, int y)
{
  uint32_t h = x * 374761393u + y * 668265263u + rnd_state;
  h = (h ^ (h >> 13)) * 1274126177u;
  h ^= h >> 16;
  return (h & 0xFFFFFF) / 16777216.0f;
}

// flying = false: пиксели на местах, true: пиксели в полёте (рисуются поверх Clawd)
static void pixelsDraw(int t, uint8_t* fb, const uint8_t* ui, bool flying)
{
  if(t >= pix_suck_start && t < pix_emit_start - 150)
  {
    int ts = t - pix_suck_start;
    for(int y = 0; y < scr_h; y++)
      for(int x = 0; x < scr_w; x++)
      {
        if(!bufGet(logo_bitmap, x, y)) continue;
        float dx = pix_tx - x, dy = pix_ty - y, len = sqrtf(dx * dx + dy * dy);
        float d = len / 86.0f * 350.0f + hash01(x, y) * 40.0f;
        if(ts < d) { if(!flying) bufSet(fb, x, y); continue; }
        float e = (ts - d) / pix_suck_fly;
        if(!flying || e >= 1.0f) continue;
        float q = e * e, s = 4.0f * e * (1.0f - e) * (hash01(y, x) - 0.5f) * 18.0f;
        if(len < 1.0f) len = 1.0f;
        bufPut(fb, (int)lroundf(x + dx * q - dy / len * s), (int)lroundf(y + dy * q + dx / len * s), true);
      }
  }
  else if(t >= pix_emit_start)
  {
    int te = t - pix_emit_start;
    for(int y = 0; y < scr_h; y++)
      for(int x = 0; x < scr_w; x++)
      {
        if(!bufGet(ui, x, y)) continue;
        float d = x * 450.0f / scr_w + hash01(x, y) * 80.0f;
        if(te < d) continue;
        float e = (te - d) / pix_emit_fly;
        if(e >= 1.0f) { if(!flying) bufSet(fb, x, y); continue; }
        if(!flying) continue;
        float q = easeOut(e);
        bufPut(fb, (int)lroundf(pix_tx + (x - pix_tx) * q),
               (int)lroundf(pix_ty + (y - pix_ty) * q - 4.0f * e * (1.0f - e) * 14.0f), true);
      }
  }
}

static void pixelsFrame(int t, uint8_t* fb, const uint8_t* ui)
{
  static const int X0 = 48, Y = 44;

  if(t < pix_suck_start) memcpy(fb, logo_bitmap, buf_size);
  else memset(fb, 0, buf_size);
  pixelsDraw(t, fb, ui, false);

  pose_t p;
  int x = X0, y = Y, lid = 10;
  if(t < 600) { x = -32 + (X0 + 32) * t / 600; y += poseWalk(p, t); p.look = 1; lid = 0; }
  else if(t < pix_suck_start) lid = (t - 600) * 10 / 150;
  else if(t < pix_emit_start - 150) poseTyping(p, t, 40);
  else if(t < pix_emit_start) { x += ((t / 40) % 2) ? 1 : -1; p.eyes = EYES_SHUT; }
  else if(t < 2460) poseTyping(p, t, 40);
  else if(t < 2600) { p.eyes = EYES_HAPPY; lid = 10 - (t - 2460) * 10 / 140; }
  else { x = X0 + (140 - X0) * (t - 2600) / 550; y += poseWalk(p, t); p.look = 1; lid = 0; }

  clawdDraw(fb, x, y, p);
  clawdLaptop(fb, x, y, lid);
  pixelsDraw(t, fb, ui, true);
}

static void introClawd(void (*frame)(int, uint8_t*, const uint8_t*), int duration,
                       uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  uint32_t start = millis();
  for(int t = 0; t < duration; t += clawd_step)
  {
    clawdGrabIface(dev_charge, is_charging, data);
    frame(t, layer_white, mask_add);
    clawdShow(layer_white);
    start = introWait(start, clawd_step);
  }
}

// ===== Запуск =====

static uint32_t introSeed()
{
  uint32_t seed = micros();
  uint8_t sd_on = 0;
  sd_softdevice_is_enabled(&sd_on);
  if(sd_on)
  {
    uint32_t hw = 0;
    sd_rand_application_vector_get((uint8_t*)&hw, sizeof(hw));
    seed ^= hw;
  }
  return seed;
}

static void introAnimation(int anim, uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  switch(anim)
  {
    case 0:  introWave(dev_charge, is_charging, data); break;
    case 1:  introBreak(dev_charge, is_charging, data); break;
    case 3:  introClawd(typeFrame, 3200, dev_charge, is_charging, data); break;
    case 4:  introClawd(laptopFrame, 2750, dev_charge, is_charging, data); break;
    case 5:  introClawd(pixelsFrame, 3150, dev_charge, is_charging, data); break;
    default: introExplosion(dev_charge, is_charging, data); break;
  }
}

void displayPlayIntroNum(int anim, uint32_t seed, uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  displayDrawLogo();
  delay(intro_logo_hold);

  rnd_state = seed;
  introAnimation(anim, dev_charge, is_charging, data);
}

bool displayIntroBegin(bool watch_button, uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  // без логотипа сразу интерфейс и ждём, только пока держат кнопку
  bool no_intro = !config().intro_logo;
  if(no_intro) displayDrawIface(dev_charge, is_charging, data);
  else displayDrawLogo();

  bool held = watch_button;
  uint32_t start = millis();
  while(millis() - start < intro_logo_hold)
  {
    if(!buttonPressed())
    {
      held = false;
      if(no_intro) break;
    }
    delay(10);
  }
  return held;
}

void displayIntroFinish(uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  const fw_config_t& cfg = config();
  if(!cfg.intro_logo) return;

  // отмеченные заставки: волна, разрушение, взрыв, затем три с Clawd - в порядке introAnimation.
  // Ни одной - после логотипа сразу интерфейс
  int enabled[6];
  int enabled_cnt = 0;
  if(cfg.intro_wave) enabled[enabled_cnt++] = 0;
  if(cfg.intro_break) enabled[enabled_cnt++] = 1;
  if(cfg.intro_explosion) enabled[enabled_cnt++] = 2;
  if(cfg.intro_clawd_type) enabled[enabled_cnt++] = 3;
  if(cfg.intro_clawd_laptop) enabled[enabled_cnt++] = 4;
  if(cfg.intro_clawd_pixels) enabled[enabled_cnt++] = 5;
  if(!enabled_cnt) return;

  uint32_t seed = introSeed();
  int anim = enabled[(seed >> 16) % enabled_cnt];
  rnd_state = seed;
  introAnimation(anim, dev_charge, is_charging, data);
}
