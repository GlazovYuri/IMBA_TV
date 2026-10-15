import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { readFileSync } from 'node:fs';
import { renderScreen, SAMPLE, SAMPLE_DEV_CHARGE, SCREEN, VALUE, SCREEN_TYPES, DEFAULT_TIMEZONE, normalizeScreen } from '../js/screens.js';

// Кадры от настоящего кода прошивки, снятые tools/render_previews.py
const fixtures = JSON.parse(readFileSync(new URL('fixtures/screens.json', import.meta.url), 'utf8'));
const source = JSON.parse(readFileSync(new URL('../../IMBA_TV/config.json', import.meta.url), 'utf8'));

const sha256 = (bytes) => createHash('sha256').update(bytes).digest('hex');

// кадр текстом для сообщения об ошибке
function ascii(oled) {
  const rows = [];
  for (let y = 0; y < 64; y++) {
    let row = '';
    for (let x = 0; x < 128; x++) row += oled.pixel(x, y) ? '#' : '.';
    rows.push(row);
  }
  return rows.join('\n');
}

test('экраны на сайте совпадают с прошивкой', () => {
  assert.ok(fixtures.cases.length > 50);
  for (const c of fixtures.cases) {
    const oled = renderScreen(c.screen, {
      data: c.data,
      devCharge: c.dev_charge,
      isCharging: c.is_charging,
      wheelVoltage: c.wheel_voltage_x10,
      firmware: fixtures.firmware,
      bootloader: fixtures.bootloader,
      time: c.clock?.unix_ms ?? null,
      timezone: c.clock?.timezone,
    });
    const got = sha256(oled.pages());
    assert.equal(got, c.sha256, `${JSON.stringify(c.screen)} батарея ${c.wheel_voltage_x10}, данные ${JSON.stringify(c.data)}:\n${ascii(oled)}`);
  }
});

test('предпросмотр рисуется на тех же данных, что и эталонные кадры', () => {
  const sample = fixtures.cases.find((c) => c.dev_charge === SAMPLE_DEV_CHARGE && c.data.speed === SAMPLE.speed && c.data.present === SAMPLE.present);
  assert.ok(sample);
  assert.deepEqual(sample.data, SAMPLE);
});

// Слоты экранов как в IMBA_TV/config.json. Сайт публикуется с основной ветки и должен проходить тесты и там,
// где прошивка ещё без слотов, поэтому редактор проверяется на этом описании, а с config.json оно сверяется,
// когда слоты в нём есть
const SLOT_DEFAULTS = [
  [SCREEN.MAIN, VALUE.CLOCK, VALUE.SPEED, VALUE.PWM, VALUE.TRIP],
  [SCREEN.GRID, VALUE.CHARGE, VALUE.VOLTAGE, VALUE.TEMPERATURE, VALUE.TRIP],
  [SCREEN.NONE, VALUE.CHARGE, VALUE.VOLTAGE, VALUE.TEMPERATURE, VALUE.TRIP],
  [SCREEN.NONE, VALUE.CHARGE, VALUE.VOLTAGE, VALUE.TEMPERATURE, VALUE.TRIP],
  [SCREEN.NONE, VALUE.CHARGE, VALUE.VOLTAGE, VALUE.TEMPERATURE, VALUE.TRIP],
];
const fields = SLOT_DEFAULTS.flatMap(([type, ...v], i) => [
  { key: `screen${i + 1}_type`, type: 'u8', default: type },
  ...v.map((d, k) => ({ key: `screen${i + 1}_v${k + 1}`, type: 'u8', default: d })),
]);
const sourceFields = source.groups.flatMap((g) => g.fields);
const configHasScreens = sourceFields.some((f) => f.key === 'screen1_type');

test('номера типов и значений как в config.json', { skip: !configHasScreens && 'в config.json этой ветки нет слотов экранов' }, () => {
  const byKey = Object.fromEntries(sourceFields.map((f) => [f.key, f]));
  for (const f of fields) assert.equal(byKey[f.key]?.default, f.default, `${f.key}: значение по умолчанию`);
  assert.ok(source.groups.some((g) => g.widget === 'screens' && g.fields.some((f) => f.key === 'screen1_type')));
  const ids = (f) => Object.fromEntries(f.options.map((o) => [o.id, o.value]));
  // сайт может знать экраны и значения, которых в этой прошивке ещё нет (их он и не предложит), но не наоборот
  for (const [id, value] of Object.entries(ids(byKey.screen1_type))) assert.equal(SCREEN[id], value, `screen1_type: ${id}`);
  for (const [id, value] of Object.entries(ids(byKey.screen1_v1))) assert.equal(VALUE[id], value, `screen1_v1: ${id}`);

  // у всех экранов одни и те же варианты: прошивка понимает номера одинаково
  for (let n = 1; n <= 5; n++) {
    assert.deepEqual(byKey[`screen${n}_type`].options, byKey.screen1_type.options);
    for (let k = 1; k <= 4; k++) assert.deepEqual(byKey[`screen${n}_v${k}`].options, byKey.screen1_v1.options);
  }
  assert.ok(!byKey.screen6_type, 'экранов не больше пяти');

  // по умолчанию два экрана: два значения с часами сверху и сетка
  const screens = [1, 2, 3, 4, 5].map((n) => ({
    type: byKey[`screen${n}_type`].default,
    v: [1, 2, 3, 4].map((k) => byKey[`screen${n}_v${k}`].default),
  }));
  assert.deepEqual(screens.map((s) => s.type), [SCREEN.MAIN, SCREEN.GRID, SCREEN.NONE, SCREEN.NONE, SCREEN.NONE]);
  assert.deepEqual(screens[0].v.slice(0, 3), [VALUE.CLOCK, VALUE.SPEED, VALUE.PWM]);
  assert.deepEqual(screens[1].v, [VALUE.CHARGE, VALUE.VOLTAGE, VALUE.TEMPERATURE, VALUE.TRIP]);
});

test('у настраиваемых мест варианты, которые прошивка не заменит', () => {
  for (const [type, info] of Object.entries(SCREEN_TYPES)) {
    info.slots.forEach((slot, i) => {
      for (const value of slot.values) {
        const v = [...info.defaults];
        v[i] = value;
        assert.deepEqual(normalizeScreen({ type: Number(type), v }).v, v, `${info.name}, ${slot.label}: ${value}`);
      }
    });
    assert.deepEqual(normalizeScreen({ type: Number(type), v: info.defaults }).v, info.defaults);
  }
  // чужое значение на экране с двумя значениями прошивка заменит
  assert.deepEqual(normalizeScreen({ type: SCREEN.MAIN, v: [VALUE.TRIP, VALUE.CHARGE, VALUE.TRIP, 0] }).v,
    [VALUE.CHARGE, VALUE.SPEED, VALUE.PWM, 0]);
});

// ---------- редактор: экраны в слотах настроек ----------

import { screenSlots, readScreens, writeScreens, moveScreen, newScreen, changedScreens, describeScreens } from '../js/screen-editor.js';

const slots = screenSlots(fields);
const defaults = () => Object.fromEntries(fields.map((f) => [f.key, f.default]));

test('экраны читаются по порядку, пустые слоты пропускаются', () => {
  assert.equal(slots.length, 5);
  const values = defaults();
  assert.deepEqual(readScreens(slots, values).map((s) => s.type), [SCREEN.MAIN, SCREEN.GRID]);
  values.screen1_type = SCREEN.NONE;
  values.screen3_type = SCREEN.INFO;
  assert.deepEqual(readScreens(slots, values).map((s) => s.type), [SCREEN.GRID, SCREEN.INFO]);
  assert.equal(changedScreens(slots, defaults()), 0);
});

test('добавление, перестановка и удаление записываются в слоты подряд', () => {
  const values = defaults();
  let screens = readScreens(slots, values);
  screens = [...screens, newScreen(SCREEN.INFO), newScreen(SCREEN.CLOCK), newScreen(SCREEN.GRID)];
  writeScreens(slots, values, screens);
  assert.equal(values.screen5_type, SCREEN.GRID);
  assert.deepEqual([1, 2, 3, 4].map((k) => values[`screen5_v${k}`]), SCREEN_TYPES[SCREEN.GRID].defaults);

  // сведения о связи - первым экраном
  screens = moveScreen(readScreens(slots, values), 2, 0);
  writeScreens(slots, values, screens);
  assert.deepEqual(readScreens(slots, values).map((s) => s.type), [SCREEN.INFO, SCREEN.MAIN, SCREEN.GRID, SCREEN.CLOCK, SCREEN.GRID]);

  // удаляем два экрана: хвост освобождается и получает значения по умолчанию
  screens = readScreens(slots, values).filter((_, i) => i !== 1 && i !== 3);
  writeScreens(slots, values, screens);
  assert.deepEqual(readScreens(slots, values).map((s) => s.type), [SCREEN.INFO, SCREEN.GRID, SCREEN.GRID]);
  assert.equal(values.screen4_type, SCREEN.NONE);
  assert.equal(values.screen5_type, SCREEN.NONE);
  for (const f of [...slots[4].v, ...slots[3].v]) assert.equal(values[f.key], f.default);
  // первое место: связь вместо двух значений, третье: сетка вместо пустого
  assert.equal(changedScreens(slots, values), 2);
  assert.match(describeScreens(slots, values), /^1\) Связь и версии: .*2\) Четыре значения: заряд колеса, напряжение колеса/);
});

test('изменение значения на экране считается одним изменённым экраном', () => {
  const values = defaults();
  const screens = readScreens(slots, values);
  screens[0].v[2] = VALUE.TEMPERATURE;
  writeScreens(slots, values, screens);
  assert.equal(changedScreens(slots, values), 1);
  // поля, которых на экране нет, не считаются
  values.screen4_v1 = VALUE.SPEED;
  assert.equal(changedScreens(slots, values), 1);
});

import { zoneBox } from '../js/screen-editor.js';

test('рамки мест обводят то, что место рисует, и не налезают друг на друга', () => {
  const overlap = (a, b) => a[0] <= b[2] && b[0] <= a[2] && a[1] <= b[3] && b[1] <= a[3];
  const all = [VALUE.SPEED, VALUE.PWM, VALUE.CHARGE, VALUE.VOLTAGE, VALUE.TEMPERATURE, VALUE.TRIP];
  const cases = [];
  for (const top of [VALUE.CHARGE, VALUE.VOLTAGE, VALUE.CLOCK]) {
    for (const l of [VALUE.SPEED, VALUE.PWM, VALUE.TEMPERATURE]) {
      for (const r of [VALUE.SPEED, VALUE.PWM, VALUE.TEMPERATURE]) cases.push({ type: SCREEN.MAIN, v: [top, l, r, 0] });
    }
  }
  for (const a of all) for (const b of all) cases.push({ type: SCREEN.GRID, v: [a, b, all[(a + 2) % 6], all[(b + 3) % 6]] });

  for (const screen of cases) {
    const oled = renderScreen(screen);
    const { slots } = SCREEN_TYPES[screen.type];
    const zones = slots.map((s, i) => {
      const box = oled.boxes.get(i);
      assert.ok(box, `${JSON.stringify(screen)}: место ${i} ничего не нарисовало`);
      // все точки места внутри его рамки
      const zone = zoneBox(box, s.zone);
      assert.ok(zone[0] <= box[0] && zone[1] <= box[1] && zone[2] >= box[2] && zone[3] >= box[3]);
      return zone;
    });
    for (let i = 0; i < zones.length; i++) {
      for (let k = i + 1; k < zones.length; k++) {
        assert.ok(!overlap(zones[i], zones[k]), `${JSON.stringify(screen)}: рамки ${i} и ${k} пересекаются`);
      }
    }
  }
});

import { addableTypes, selectableValues } from '../js/screen-editor.js';

test('часы предлагаются, только если прошивка их знает', () => {
  assert.deepEqual(addableTypes(slots), [SCREEN.MAIN, SCREEN.GRID, SCREEN.INFO, SCREEN.CLOCK]);
  // прошивка до часов: варианты screenN_type без CLOCK
  const old = screenSlots(fields.map((f) => (f.key.endsWith('_type')
    ? { ...f, options: [0, 1, 2, 3].map((value) => ({ value })) }
    : f)));
  assert.deepEqual(addableTypes(old), [SCREEN.MAIN, SCREEN.GRID, SCREEN.INFO]);
});

test('часы в верхней строке предлагаются, только если прошивка их знает', () => {
  const top = SCREEN_TYPES[SCREEN.MAIN].slots[0].values;
  const withOptions = (values) => fields.map((f) => (/_v\d$/.test(f.key) ? { ...f, options: values.map((value) => ({ value })) } : f));
  assert.deepEqual(selectableValues(screenSlots(withOptions(Object.values(VALUE))), top), [VALUE.CHARGE, VALUE.VOLTAGE, VALUE.CLOCK]);
  // прошивка до часов в верхней строке: варианты screenN_vK без CLOCK
  const old = screenSlots(withOptions([0, 1, 2, 3, 4, 5]));
  assert.deepEqual(selectableValues(old, top), [VALUE.CHARGE, VALUE.VOLTAGE]);
  assert.deepEqual(selectableValues(old, SCREEN_TYPES[SCREEN.GRID].slots[0].values), SCREEN_TYPES[SCREEN.GRID].slots[0].values);
});

test('часы в верхней строке: время из снимка и заряд колеса в правом углу', () => {
  const screen = { type: SCREEN.MAIN, v: [VALUE.CLOCK, VALUE.SPEED, VALUE.PWM, VALUE.TRIP] };
  const time = Date.UTC(2026, 9, 12, 12, 42, 30);
  const top = (o) => Array.from({ length: 10 }, (_, y) => Array.from({ length: 128 }, (_, x) => o.pixel(x, y)).join('')).join('\n');
  const none = renderScreen(screen);
  const at = renderScreen(screen, { time, timezone: DEFAULT_TIMEZONE });
  assert.notEqual(top(at), top(none));
  // время на сайте - в выбранном поясе, как у экрана часов
  assert.notEqual(top(at), top(renderScreen(screen, { time, timezone: DEFAULT_TIMEZONE + 4 })));
  // заряд колеса доходит до правого края: последний столбец - промежуток за знаком «%»
  const right = (o) => Math.max(...Array.from({ length: 10 }, (_, y) => Array.from({ length: 128 }, (_, x) => (o.pixel(x, y) ? x : -1))).flat());
  assert.equal(right(at), 126);
  // крупные значения те же, что при заряде сверху
  const big = (o) => Array.from({ length: 54 }, (_, y) => Array.from({ length: 128 }, (_, x) => o.pixel(x, y + 10)).join('')).join('\n');
  assert.equal(big(at), big(renderScreen({ ...screen, v: [VALUE.CHARGE, VALUE.SPEED, VALUE.PWM, VALUE.TRIP] })));
});

test('часы без времени от телефона показывают прочерки и не зависят от связи', () => {
  const ascii = (o) => Array.from({ length: 64 }, (_, y) => Array.from({ length: 128 }, (_, x) => o.pixel(x, y)).join('')).join('\n');
  const none = renderScreen({ type: SCREEN.CLOCK, v: [0, 0, 0, 0] });
  assert.equal(ascii(none), ascii(renderScreen({ type: SCREEN.CLOCK, v: [0, 0, 0, 0] }, { data: { ...SAMPLE, is_connected: false } })));
  const time = Date.UTC(2026, 9, 12, 12, 0, 30);
  assert.notEqual(ascii(renderScreen({ type: SCREEN.CLOCK, v: [0, 0, 0, 0] }, { time })), ascii(none));
});

const timezoneField = sourceFields.find((f) => f.key === 'timezone');

test('подпись пояса совпадает с его значением: сдвиг в четвертях часа плюс 48', { skip: !timezoneField && 'в config.json этой ветки нет часового пояса' }, () => {
  const tz = timezoneField;
  assert.equal(tz.options.find((o) => o.value === tz.default)?.label.split(',')[0], 'UTC+3');
  for (const o of tz.options) {
    const hours = o.label === 'UTC' || o.label.startsWith('UTC,') ? 0 : Number(o.label.match(/^UTC([+-]\d+)/)[1]);
    assert.equal(o.value, 48 + hours * 4, o.label);
  }
});
