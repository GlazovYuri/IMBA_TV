#include "animations.h"
#include "display.h"
#include "config.h"
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

//   0 - волна с пузырями, 1 - разрушение блока, 2 - взрыв TNT (Minecraft).

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

void displayPlayIntroNum(int anim, uint32_t seed, uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  displayDrawLogo();
  delay(intro_logo_hold);

  rnd_state = seed;
  switch(anim)
  {
    case 0:  introWave(dev_charge, is_charging, data); break;
    case 1:  introBreak(dev_charge, is_charging, data); break;
    default: introExplosion(dev_charge, is_charging, data); break;
  }
}

void displayPlayIntro(uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  uint8_t intro = config().intro;
  if(intro == CONFIG_INTRO_NONE) return;
  if(intro == CONFIG_INTRO_LOGO)
  {
    displayDrawLogo();
    delay(intro_logo_hold);
    return;
  }

  uint32_t seed = introSeed();
  // волна, разрушение, взрыв - в порядке displayPlayIntroNum
  int anim = (intro == CONFIG_INTRO_RANDOM) ? (seed >> 16) % 3 : intro - CONFIG_INTRO_WAVE;
  displayPlayIntroNum(anim, seed, dev_charge, is_charging, data);
}
