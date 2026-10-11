import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { readFileSync } from 'node:fs';
import { renderScreen, SAMPLE, SAMPLE_DEV_CHARGE, SCREEN, VALUE, SCREEN_TYPES, normalizeScreen } from '../js/screens.js';

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

test('номера типов и значений как в config.json', () => {
  const fields = source.groups.flatMap((g) => g.fields);
  const byKey = Object.fromEntries(fields.map((f) => [f.key, f]));
  const ids = (f) => Object.fromEntries(f.options.map((o) => [o.id, o.value]));
  assert.deepEqual(ids(byKey.screen1_type), SCREEN);
  assert.deepEqual(ids(byKey.screen1_v1), VALUE);

  // у всех экранов одни и те же варианты: прошивка понимает номера одинаково
  for (let n = 1; n <= 5; n++) {
    assert.deepEqual(byKey[`screen${n}_type`].options, byKey.screen1_type.options);
    for (let k = 1; k <= 4; k++) assert.deepEqual(byKey[`screen${n}_v${k}`].options, byKey.screen1_v1.options);
  }
  assert.ok(!byKey.screen6_type, 'экранов не больше пяти');

  // по умолчанию экраны как до настройки: два значения с зарядом, с напряжением, сетка, связь и версии
  const screens = [1, 2, 3, 4, 5].map((n) => ({
    type: byKey[`screen${n}_type`].default,
    v: [1, 2, 3, 4].map((k) => byKey[`screen${n}_v${k}`].default),
  }));
  assert.deepEqual(screens.map((s) => s.type), [SCREEN.MAIN, SCREEN.MAIN, SCREEN.GRID, SCREEN.INFO, SCREEN.NONE]);
  assert.deepEqual(screens[0].v.slice(0, 3), [VALUE.CHARGE, VALUE.SPEED, VALUE.PWM]);
  assert.deepEqual(screens[1].v.slice(0, 3), [VALUE.VOLTAGE, VALUE.SPEED, VALUE.PWM]);
  assert.deepEqual(screens[2].v, [VALUE.CHARGE, VALUE.VOLTAGE, VALUE.TEMPERATURE, VALUE.TRIP]);
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

const fields = source.groups.flatMap((g) => g.fields);
const slots = screenSlots(fields);
const defaults = () => Object.fromEntries(fields.map((f) => [f.key, f.default]));

test('экраны читаются по порядку, пустые слоты пропускаются', () => {
  assert.equal(slots.length, 5);
  const values = defaults();
  assert.deepEqual(readScreens(slots, values).map((s) => s.type), [SCREEN.MAIN, SCREEN.MAIN, SCREEN.GRID, SCREEN.INFO]);
  values.screen2_type = SCREEN.NONE;
  assert.deepEqual(readScreens(slots, values).map((s) => s.type), [SCREEN.MAIN, SCREEN.GRID, SCREEN.INFO]);
  assert.equal(changedScreens(slots, defaults()), 0);
});

test('добавление, перестановка и удаление записываются в слоты подряд', () => {
  const values = defaults();
  let screens = readScreens(slots, values);
  screens = [...screens, newScreen(SCREEN.GRID)];
  writeScreens(slots, values, screens);
  assert.equal(values.screen5_type, SCREEN.GRID);
  assert.deepEqual([1, 2, 3, 4].map((k) => values[`screen5_v${k}`]), SCREEN_TYPES[SCREEN.GRID].defaults);

  // сведения о связи - первым экраном
  screens = moveScreen(readScreens(slots, values), 3, 0);
  writeScreens(slots, values, screens);
  assert.deepEqual(readScreens(slots, values).map((s) => s.type), [SCREEN.INFO, SCREEN.MAIN, SCREEN.MAIN, SCREEN.GRID, SCREEN.GRID]);

  // удаляем два экрана: хвост освобождается и получает значения по умолчанию
  screens = readScreens(slots, values).filter((_, i) => i !== 1 && i !== 2);
  writeScreens(slots, values, screens);
  assert.deepEqual(readScreens(slots, values).map((s) => s.type), [SCREEN.INFO, SCREEN.GRID, SCREEN.GRID]);
  assert.equal(values.screen4_type, SCREEN.NONE);
  assert.equal(values.screen5_type, SCREEN.NONE);
  for (const f of [...slots[4].v, ...slots[3].v]) assert.equal(values[f.key], f.default);
  assert.ok(changedScreens(slots, values) >= 3);
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
