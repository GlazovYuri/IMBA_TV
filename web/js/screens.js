// Экраны дисплея для редактора на сайте: тот же код, что в прошивке (IMBA_TV/screen_main.cpp, screen_grid.cpp,
// screen_info.cpp, screen_clock.cpp, screen_text.cpp, euc_values.cpp), только рисует в Oled из oled.js.
// При изменении экранов в прошивке поменяйте и этот файл, затем снимите кадры: python3 tools/render_previews.py.
// Тест web/test/screens.test.mjs сверяет отрисовку с кадрами от настоящей прошивки.

import { Oled, OLED_FILL, OLED_STROKE, BUF_ADD, BUF_REPLACE, WIDTH } from './oled.js';

// типы экранов и значения - номера из config.json (screenN_type, screenN_v1..v4)
export const SCREEN = { NONE: 0, MAIN: 1, GRID: 2, INFO: 3, CLOCK: 4 };
export const VALUE = { SPEED: 0, PWM: 1, CHARGE: 2, VOLTAGE: 3, TEMPERATURE: 4, TRIP: 5 };
const HAS = { SPEED: 1, PWM: 2, VOLTAGE: 4, CHARGE: 8, TEMPERATURE: 16, TRIP: 32 };

export const VALUE_NAMES = {
  [VALUE.SPEED]: 'Скорость',
  [VALUE.PWM]: 'ШИМ',
  [VALUE.CHARGE]: 'Заряд колеса',
  [VALUE.VOLTAGE]: 'Напряжение колеса',
  [VALUE.TEMPERATURE]: 'Температура',
  [VALUE.TRIP]: 'Пробег',
};

const BIG_VALUES = [VALUE.SPEED, VALUE.PWM, VALUE.TEMPERATURE];
const ALL_VALUES = [VALUE.SPEED, VALUE.PWM, VALUE.CHARGE, VALUE.VOLTAGE, VALUE.TEMPERATURE, VALUE.TRIP];

// Что настраивается на экране: поле screenN_v{slot + 1}, место на дисплее (x, y, ширина, высота) и варианты
export const SCREEN_TYPES = {
  [SCREEN.MAIN]: {
    name: 'Два значения',
    about: 'Крупно два значения, сверху заряд или напряжение колеса',
    defaults: [VALUE.CHARGE, VALUE.SPEED, VALUE.PWM, VALUE.TRIP],
    slots: [
      { label: 'Верхняя строка', zone: [46, 0, 82, 11], values: [VALUE.CHARGE, VALUE.VOLTAGE] },
      { label: 'Крупно слева', zone: [0, 14, 72, 50], values: BIG_VALUES },
      { label: 'Крупно справа', zone: [72, 14, 56, 50], values: BIG_VALUES },
    ],
  },
  [SCREEN.GRID]: {
    name: 'Четыре значения',
    about: 'Сетка 2 × 2: любые значения в любом порядке',
    defaults: [VALUE.CHARGE, VALUE.VOLTAGE, VALUE.TEMPERATURE, VALUE.TRIP],
    slots: [
      { label: 'Слева сверху', zone: [0, 0, 64, 32], values: ALL_VALUES },
      { label: 'Справа сверху', zone: [64, 0, 64, 32], values: ALL_VALUES },
      { label: 'Слева снизу', zone: [0, 32, 64, 32], values: ALL_VALUES },
      { label: 'Справа снизу', zone: [64, 32, 64, 32], values: ALL_VALUES },
    ],
  },
  [SCREEN.INFO]: {
    name: 'Связь и версии',
    about: 'Задержка и частота данных от телефона, версии прошивки и загрузчика',
    defaults: [VALUE.CHARGE, VALUE.VOLTAGE, VALUE.TEMPERATURE, VALUE.TRIP],
    slots: [],
  },
  [SCREEN.CLOCK]: {
    name: 'Часы',
    about: 'Крупно время, под ним день недели и дата. Время приходит из LoEUC, пояс выбирается ниже',
    defaults: [VALUE.CHARGE, VALUE.VOLTAGE, VALUE.TEMPERATURE, VALUE.TRIP],
    slots: [],
  },
};

// пояс по умолчанию (config.json, timezone): сдвиг от UTC в четвертях часа плюс 48, здесь UTC+3
export const DEFAULT_TIMEZONE = 60;

// Данные для предпросмотра - те же, что у стенда tools/previews/harness.cpp
export const SAMPLE = {
  is_connected: true,
  present: 63,
  speed: 27,
  pwm: 41,
  charge: 76,
  voltage_x10: 1418,
  temperature_x10: 384,
  trip_x10: 124,
  frame_interval_ms: 120,
  frame_last_interval_ms: 95,
};
export const SAMPLE_DEV_CHARGE = 82;

// Значения экрана, которые прошивка покажет на самом деле (display.cpp): у экрана с двумя значениями свои варианты
export function normalizeScreen(screen) {
  const v = [...screen.v];
  if (screen.type === SCREEN.MAIN) {
    if (v[0] !== VALUE.VOLTAGE) v[0] = VALUE.CHARGE;
    if (!BIG_VALUES.includes(v[1])) v[1] = VALUE.SPEED;
    if (!BIG_VALUES.includes(v[2])) v[2] = VALUE.PWM;
  }
  return { type: screen.type, v };
}

// ---------- как в C: целочисленное деление и constrain ----------

const div = (a, b) => Math.trunc(a / b);
const constrain = (x, lo, hi) => (x < lo ? lo : (x > hi ? hi : x));
const x10 = (n) => `${div(n, 10)}.${n % 10}`;

// ---------- euc_values.cpp ----------

export function valueText(value, d) {
  switch (value) {
    case VALUE.SPEED: return d.present & HAS.SPEED ? String(constrain(d.speed, 0, 999)) : '--';
    case VALUE.PWM: return d.present & HAS.PWM ? String(d.pwm) : '--';
    case VALUE.CHARGE: return d.present & HAS.CHARGE ? String(constrain(d.charge, 0, 100)) : '--';
    case VALUE.VOLTAGE: return d.present & HAS.VOLTAGE ? x10(d.voltage_x10) : '--.-';
    case VALUE.TEMPERATURE: {
      if (!(d.present & HAS.TEMPERATURE)) return '--';
      let t = d.temperature_x10;
      t = t >= 0 ? div(t + 5, 10) : -div(-t + 5, 10);
      return String(constrain(t, -99, 999));
    }
    case VALUE.TRIP:
      if (!(d.present & HAS.TRIP)) return '--.-';
      return d.trip_x10 >= 10000 ? String(div(d.trip_x10, 10)) : x10(d.trip_x10);
    default: return '--';
  }
}

// ---------- screen_text.cpp ----------

function bigWidth(text) {
  let width = 0;
  for (const ch of text) width += ch === '.' ? 4 : 12;
  return width - 2;
}

function textBig(oled, right, y, text) {
  let x = Math.max(right - bigWidth(text) + 1, 0);
  oled.setScale(2);
  for (const ch of text) {
    if (ch === '.') {
      oled.rect(x, y + 12, x + 1, y + 13, OLED_FILL);
      x += 4;
      continue;
    }
    oled.setCursorXY(x, y);
    oled.print(ch);
    x += 12;
  }
}

function textUnit(oled, x, y, unit) {
  oled.setScale(1);
  oled.setCursorXY(x, y + 7);
  oled.print(unit);
}

// ---------- screen_main.cpp ----------

const CHARGING_BITMAP = [0xff, 0x81, 0xb1, 0x99, 0x8d, 0x9d, 0xb9, 0xb1, 0x99, 0x8d, 0x81, 0xff, 0x3c, 0x3c];

function drawDevCharge(oled, x, y, charge, isCharging) {
  charge = constrain(charge, 0, 100);
  if (isCharging) oled.drawBitmap(x, y, CHARGING_BITMAP, 14, 8);
  else {
    oled.setCursorXY(x, y);
    oled.drawByte(0xff);
    const level = div(charge, 10);
    for (let i = 0; i < 10; i++) oled.drawByte(i < level ? 0xff : 0x81);
    oled.drawByte(0xff);
    oled.drawByte(0x3c);
    oled.drawByte(0x3c);
  }
  oled.setCursorXY(x + 17, y);
  oled.setScale(1);
  oled.print(charge);
  oled.print('%');
}

function drawEucBar(oled, x, y, charge, cells) {
  charge = constrain(charge, 0, 100);
  oled.setCursorXY(x, y);
  oled.drawByte(0xff);
  const level = div(charge * cells, 100);
  for (let i = 0; i < cells; i++) {
    oled.drawByte(0x81);
    oled.drawByte(i < level ? 0xff : 0x81);
  }
  oled.drawByte(0xff);
  oled.drawByte(0x3c);
  oled.drawByte(0x3c);
}

function drawEucCharge(oled, x, y, charge) {
  charge = constrain(charge, 0, 100);
  drawEucBar(oled, x, y, charge, 25);
  oled.setCursorXY(x + 57, y);
  oled.setScale(1);
  oled.print(charge);
  oled.print('%');
}

function drawEucVoltage(oled, x, y, d, wheelVoltage) {
  const text = d.present & HAS.VOLTAGE ? x10(d.voltage_x10) : '--.-';
  const maxText = x10(wheelVoltage);
  const fieldX = WIDTH - (maxText.length + 1) * 6;
  const cells = div(fieldX - 3 - x - 4, 2);
  drawEucBar(oled, x, y, d.charge, cells);
  oled.setCursorXY(WIDTH - (text.length + 1) * 6, y);
  oled.setScale(1);
  oled.print(text);
  oled.print('В');
}

const BIG_LABELS = { [VALUE.SPEED]: 'скорость', [VALUE.PWM]: 'ШИМ', [VALUE.TEMPERATURE]: 'темп' };

function centerX(center2, width, left, right) {
  return constrain(div(center2 - width + 2, 2), left, right - width + 1);
}

function drawBigValue(oled, left, right, center2, y, value, d) {
  const text = valueText(value, d);
  const chars = [...text].length;
  let scale = 4;
  while (scale > 2 && chars * 6 * scale - scale > right - left + 1) scale--;
  let width = chars * 6 * scale - scale;
  const numberCenter2 = scale === 4 ? center2 : left + right;
  oled.setCursorXY(centerX(numberCenter2, width, left, right), y + 7 * (4 - scale));
  oled.setScale(scale);
  oled.print(text);

  const label = BIG_LABELS[value] ?? '';
  width = [...label].length * 6 - 1;
  oled.setCursorXY(centerX(center2, width, 0, WIDTH - 1), y + 38);
  oled.setScale(1);
  oled.print(label);
}

// oled.tag - номер места на экране (как поля v1..v4): по нему редактор обводит то, что место рисует
function mainDraw(oled, devCharge, isCharging, d, [top, left, right], wheelVoltage) {
  drawDevCharge(oled, 0, 1, devCharge, isCharging);
  oled.tag = 0;
  if (top === VALUE.VOLTAGE) drawEucVoltage(oled, 47, 1, d, wheelVoltage);
  else drawEucCharge(oled, 47, 1, d.charge);
  oled.tag = 1;
  drawBigValue(oled, 0, 71, 67, 18, left, d);
  oled.tag = 2;
  drawBigValue(oled, 72, WIDTH - 1, 187, 18, right, d);
  oled.tag = null;
}

function noLinkDraw(oled, devCharge, isCharging) {
  drawDevCharge(oled, 0, 1, devCharge, isCharging);
  oled.setCursor(10, 3);
  oled.setScale(2);
  oled.print('нет связи');
}

// ---------- screen_grid.cpp ----------

const UNIT_GAP = 3;
const COLUMN_GAP = 6;
const UNIT_WIDTH = { [VALUE.SPEED]: 24, [VALUE.PWM]: 18, [VALUE.TEMPERATURE]: 10, [VALUE.TRIP]: 12 };
const UNIT_TEXT = { [VALUE.SPEED]: 'км/ч', [VALUE.PWM]: 'ШИМ', [VALUE.CHARGE]: '%', [VALUE.VOLTAGE]: 'В', [VALUE.TRIP]: 'км' };

function drawUnit(oled, x, y, value) {
  if (value === VALUE.TEMPERATURE) {
    oled.rect(x, y + 7, x + 2, y + 9, OLED_STROKE);
    textUnit(oled, x + 4, y, 'C');
  } else if (UNIT_TEXT[value]) textUnit(oled, x, y, UNIT_TEXT[value]);
}

function numberWidth(value, wheelVoltage) {
  if (value === VALUE.VOLTAGE) return bigWidth(x10(wheelVoltage));
  if (value === VALUE.TRIP) return bigWidth('999.9');
  return bigWidth('100');
}

const placeLeft = (numberW) => ({ right: numberW + 2, unitX: numberW + 2 + UNIT_GAP, fitText: false });
const placeRight = (unitW) => ({ right: WIDTH - 1 - unitW - UNIT_GAP, unitX: WIDTH - 1 - unitW, fitText: false });

function fits(left, leftUnitW, right, rightNumberW) {
  const leftLast = left.unitX + leftUnitW - 2;
  const rightFirst = right.right - rightNumberW + 1;
  return rightFirst - leftLast - 1 >= COLUMN_GAP;
}

function gridDraw(oled, d, values, wheelVoltage) {
  const rowY = [7, 37];
  const numberW = values.map((v) => numberWidth(v, wheelVoltage));
  const unitW = values.map((v) => UNIT_WIDTH[v] ?? 6);

  const cells = [];
  cells[0] = cells[2] = placeLeft(Math.max(numberW[0], numberW[2]));
  cells[1] = cells[3] = placeRight(Math.max(unitW[1], unitW[3]));
  if (!fits(cells[0], Math.max(unitW[0], unitW[2]), cells[1], Math.max(numberW[1], numberW[3]))) {
    for (let i = 0; i < 4; i += 2) {
      cells[i] = placeLeft(numberW[i]);
      cells[i + 1] = placeRight(unitW[i + 1]);
      cells[i].fitText = !fits(cells[i], unitW[i], cells[i + 1], numberW[i + 1]);
    }
  }

  for (let i = 0; i < 4; i++) {
    const y = rowY[i >> 1];
    let text = valueText(values[i], d);
    const crowded = cells[i & ~1].fitText;
    if (crowded && values[i] === VALUE.TRIP && (d.present & HAS.TRIP) && d.trip_x10 >= 1000) text = String(div(d.trip_x10, 10));
    const width = bigWidth(text);
    let c = cells[i].fitText ? placeLeft(width) : cells[i];
    if (c.right - width + 1 < 3) c = placeLeft(width);
    oled.tag = i;
    textBig(oled, c.right, y, text);
    drawUnit(oled, c.unitX, y, values[i]);
    oled.tag = null;
  }
}

// ---------- screen_info.cpp ----------

function drawRow(oled, y, label, value) {
  oled.setCursorXY(1, y);
  oled.print(label);
  oled.setCursorXY(WIDTH - [...value].length * 6, y);
  oled.print(value);
}

function infoDraw(oled, d, linkOk, firmware, bootloader) {
  let text;
  let intervalUnit = 'мс';
  if (!linkOk) text = '--';
  else if (d.frame_last_interval_ms < 1000) text = String(d.frame_last_interval_ms);
  else {
    const s = Math.min(div(d.frame_last_interval_ms + 50, 100), 999);
    text = x10(s);
    intervalUnit = 'с';
  }
  textBig(oled, 36, 10, text);
  textUnit(oled, 39, 10, intervalUnit);

  text = linkOk ? x10(div(10000 + div(d.frame_interval_ms, 2), d.frame_interval_ms)) : '--.-';
  textBig(oled, 112, 10, text);
  textUnit(oled, 115, 10, 'Гц');

  oled.textMode(BUF_ADD);
  oled.setScale(1);
  oled.setCursorXY(1, 0);
  oled.print('Задержка');
  oled.setCursorXY(66, 0);
  oled.print('Частота');
  drawRow(oled, 37, 'Прошивка', firmware);
  drawRow(oled, 50, 'Загрузчик', bootloader || '--');
  oled.textMode(BUF_REPLACE);

  oled.fastLineH(31, 0, WIDTH - 1);
}

// ---------- screen_clock.cpp ----------

const WEEKDAYS = ['пн', 'вт', 'ср', 'чт', 'пт', 'сб', 'вс'];
const two = (n) => String(n).padStart(2, '0');

function printCentered(oled, y, text) {
  oled.setScale(2);
  oled.setCursorXY(div(WIDTH - ([...text].length * 12 - 2), 2), y);
  oled.print(text);
}

// time - Unix-время в мс из снимка LoEUC или null, timezone - значение настройки
function clockDraw(oled, time, timezone) {
  let hours = '--';
  let minutes = '--';
  let date = '--.--';
  if (time != null) {
    const localS = Math.floor(time / 1000) + timezone * 900 - 48 * 900;
    const secs = localS % 86400;
    const days = Math.floor(localS / 86400);
    hours = two(div(secs, 3600));
    minutes = two(div(secs, 60) % 60);

    // civil_from_days, как в прошивке
    const z = days + 719468;
    const era = div(z, 146097);
    const doe = z - era * 146097;
    const yoe = div(doe - div(doe, 1460) + div(doe, 36524) - div(doe, 146096), 365);
    const doy = doe - (365 * yoe + div(yoe, 4) - div(yoe, 100));
    const mp = div(5 * doy + 2, 153);
    const day = doy - div(153 * mp + 2, 5) + 1;
    const month = mp < 10 ? mp + 3 : mp - 9;
    date = `${WEEKDAYS[(days + 3) % 7]} ${two(day)}.${two(month)}`;
  }

  oled.setScale(4);
  oled.setCursorXY(12, 6);
  oled.print(hours);
  oled.setCursorXY(72, 6);
  oled.print(minutes);
  oled.rect(62, 13, 65, 16, OLED_FILL);
  oled.rect(62, 23, 65, 26, OLED_FILL);

  printCentered(oled, 44, date);
}

// ---------- display.cpp ----------

// Кадр экрана { type, v: [4 значения] } как на дисплее. В oled.boxes - рамки настраиваемых мест:
// номер места -> [x0, y0, x1, y1] зажжённых им точек
export function renderScreen(screen, {
  data = SAMPLE,
  devCharge = SAMPLE_DEV_CHARGE,
  isCharging = false,
  wheelVoltage = 1512,
  firmware = '',
  bootloader = '',
  time = null,
  timezone = DEFAULT_TIMEZONE,
} = {}) {
  const oled = new Oled();
  const s = normalizeScreen(screen);
  // кадр от телефона пришёл только что: связь показывается, если он подключён и частота уже измерена
  const linkOk = data.is_connected && data.frame_interval_ms !== 0;
  if (s.type === SCREEN.INFO) infoDraw(oled, data, linkOk, firmware, bootloader);
  else if (s.type === SCREEN.CLOCK) clockDraw(oled, time, timezone);
  else if (!data.is_connected) noLinkDraw(oled, devCharge, isCharging);
  else if (s.type === SCREEN.GRID) gridDraw(oled, data, s.v, wheelVoltage);
  else mainDraw(oled, devCharge, isCharging, data, s.v, wheelVoltage);
  return oled;
}
