import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync, existsSync } from 'node:fs';
import {
  findConfig, readValues, defaultValues, checkValue, isVisible, applyValues, mergeValues,
  hasOwnSettings, initialValues, savedValues,
} from '../js/config.js';
import { readZip, loadFirmware, makeUf2, withImageCrc, crc16, FirmwareError } from '../js/firmware.js';
import { schema, fields, defaults, makeBlock, withBlock } from './config-block.mjs';

const source = JSON.parse(readFileSync(new URL('../../IMBA_TV/config.json', import.meta.url), 'utf8'));
const zip = readFileSync(new URL('fixtures/app.zip', import.meta.url));
const files = await readZip(zip.buffer.slice(zip.byteOffset, zip.byteOffset + zip.byteLength));
const appBin = files.get('app.bin');
const appDat = files.get('app.dat');

test('встроенное описание совпадает с config.json', () => {
  const strip = (s) => s.groups.map((g) => ({ ...g, fields: g.fields.map(({ offset, size, ...f }) => f) }));
  assert.deepEqual(strip(schema), strip(source));
  assert.deepEqual(defaults, fields.map((f) => Number(f.default)));
});

test('у каждого превью в config.json есть гифка и кадр для режима без анимации', () => {
  const previews = source.groups.flatMap((g) => g.fields).filter((f) => f.preview).map((f) => f.preview);
  assert.ok(previews.length > 0);
  for (const name of previews) {
    for (const ext of ['gif', 'png']) {
      assert.ok(existsSync(new URL(`../previews/${name}.${ext}`, import.meta.url)), `нет web/previews/${name}.${ext}`);
    }
  }
});

test('подпункт зависит от галочки выше в той же группе', () => {
  for (const group of source.groups) {
    group.fields.forEach((f, i) => {
      if (!f.subitem) return;
      const parents = Object.keys(f.depends ?? {});
      assert.ok(parents.length > 0, `${f.key}: у подпункта нет depends`);
      for (const key of parents) {
        const parent = group.fields.slice(0, i).find((g) => g.key === key);
        assert.ok(parent && parent.type === 'bool', `${f.key}: подпункт должен зависеть от галочки выше в группе «${group.title}»`);
      }
    });
  }
});

test('прошивка без блока настроек', () => {
  assert.equal(findConfig(appBin), null);
});

test('значения по умолчанию читаются из образа', () => {
  const cfg = findConfig(withBlock(appBin));
  assert.equal(cfg.fields.length, fields.length);
  assert.deepEqual(readValues(withBlock(appBin), cfg), defaultValues(cfg));
});

test('настройки записываются только в блок', () => {
  const bin = withBlock(appBin, 2000);
  const cfg = findConfig(bin);
  const values = { ...defaultValues(cfg), pwm_alarm: false, brightness: 40, power_off_ms: 2500, intro_break: false };
  const out = applyValues(bin, cfg, values);

  assert.deepEqual(readValues(out, cfg), values);
  assert.deepEqual(readValues(bin, cfg), defaultValues(cfg), 'исходный образ не меняется');
  const changed = [...out.keys()].filter((i) => out[i] !== bin[i]);
  assert.ok(changed.every((i) => i >= cfg.offset && i < cfg.offset + cfg.size));
  assert.ok(changed.length > 0);
});

test('недопустимые значения отклоняются', () => {
  const byKey = Object.fromEntries(fields.map((f) => [f.key, f]));
  assert.equal(checkValue(byKey.brightness, '55'), 55);
  assert.throws(() => checkValue(byKey.brightness, 0), /от 5 до 100/);
  assert.throws(() => checkValue(byKey.power_on_ms, '1.5'), /целое/);
  assert.throws(() => checkValue(byKey.pwm_alarm_source, 9), /недопустимое/);
  assert.equal(checkValue(byKey.wheel_voltage_x10, '840'), 840);
  assert.throws(() => checkValue(byKey.ble_update, 1), FirmwareError);

  const cfg = findConfig(withBlock(appBin));
  assert.throws(() => applyValues(withBlock(appBin), cfg, { ...defaultValues(cfg), brightness: 300 }), FirmwareError);
});

test('зависимые поля', () => {
  const threshold = fields.find((f) => f.key === 'pwm_alarm_threshold');
  assert.equal(isVisible(threshold, { pwm_alarm: true, pwm_alarm_source: 0 }), false);
  assert.equal(isVisible(threshold, { pwm_alarm: true, pwm_alarm_source: 1 }), true);
  assert.equal(isVisible(threshold, { pwm_alarm: false, pwm_alarm_source: 1 }), false);

  // заставки играют только после логотипа
  const wave = fields.find((f) => f.key === 'intro_wave');
  assert.equal(isVisible(wave, { intro_logo: true, intro_clawd: false }), true);
  assert.equal(isVisible(wave, { intro_logo: false, intro_clawd: false }), false);

  // пасхалка Clawd заменяет обычные заставки, сама она тоже только после логотипа
  const clawd = fields.find((f) => f.key === 'intro_clawd');
  assert.equal(clawd.default, false);
  assert.equal(isVisible(wave, { intro_logo: true, intro_clawd: true }), false);
  assert.equal(isVisible(clawd, { intro_logo: true }), true);
  assert.equal(isVisible(clawd, { intro_logo: false }), false);
});

test('сохранённые настройки применяются, если подходят прошивке', () => {
  const cfg = findConfig(withBlock(appBin));
  const merged = mergeValues(cfg, defaultValues(cfg), { brightness: 30, pwm_alarm_source: 42, unknown: 1, intro_wave: false });
  assert.equal(merged.brightness, 30);
  assert.equal(merged.pwm_alarm_source, 0);
  assert.equal(merged.intro_wave, false);
  assert.ok(!('unknown' in merged));
});

test('настройки, записанные в файл, важнее сохранённых в браузере', () => {
  const bin = withBlock(appBin);
  const cfg = findConfig(bin);
  const saved = { brightness: 90, power_off_ms: 1000 };

  // в блоке значения по умолчанию (релиз, сборка из Arduino IDE): берутся сохранённые в браузере
  assert.equal(hasOwnSettings(cfg, readValues(bin, cfg)), false);
  assert.equal(initialValues(cfg, bin, saved).brightness, 90);

  // «UF2 с настройками»: в блоке свои значения, они и показываются
  const own = applyValues(bin, cfg, { ...defaultValues(cfg), brightness: 40, power_off_ms: 3000 });
  assert.equal(hasOwnSettings(cfg, readValues(own, cfg)), true);
  const values = initialValues(cfg, own, saved);
  assert.equal(values.brightness, 40);
  assert.equal(values.power_off_ms, 3000);
});

test('в браузере запоминаются только изменённые настройки', () => {
  const cfg = findConfig(withBlock(appBin));
  const values = { ...defaultValues(cfg), brightness: 40 };

  // значения по умолчанию не запоминаются, настройки других версий прошивки остаются
  const saved = savedValues(cfg, { intro: 3, power_off_ms: 1000, ble_update: false }, values);
  assert.deepEqual(saved, { intro: 3, brightness: 40 });

  // нетронутая настройка получает новое значение по умолчанию, если оно изменится в новой версии
  const next = { ...cfg, fields: cfg.fields.map((f) => (f.key === 'power_off_ms' ? { ...f, default: 1500 } : f)) };
  assert.equal(mergeValues(next, defaultValues(next), saved).power_off_ms, 1500);

  // запомненное раньше значение по умолчанию забывается при следующем сохранении, остальные остаются
  const old = { brightness: 100, power_off_ms: 2000 };
  assert.deepEqual(savedValues(cfg, old, old), { power_off_ms: 2000 });
});

test('повреждённый блок настроек', () => {
  const block = makeBlock();
  block[block.length - 5] = 0x7b; // ломаем JSON
  assert.throws(() => findConfig(withBlock(appBin, 1000, block)), /повреждён/);
  const short = makeBlock().subarray(0, 30);
  assert.throws(() => findConfig(withBlock(appBin, 1000, short).subarray(0, 1030)), /повреждён/);
});

test('настроенная прошивка проходит проверку и через UF2', async () => {
  const bin = withBlock(appBin, 2000);
  const cfg = findConfig(bin);
  const values = { ...defaultValues(cfg), pwm_alarm_source: 1, pwm_alarm_threshold: 70 };
  const out = applyValues(bin, cfg, values);
  const dat = withImageCrc(appDat, out);
  assert.deepEqual(dat.subarray(0, -2), appDat.subarray(0, -2));
  assert.equal(dat[dat.length - 2] | (dat[dat.length - 1] << 8), crc16(out));

  const fw = await loadFirmware('settings.uf2', makeUf2(out).buffer);
  assert.deepEqual(fw.bin.subarray(0, out.length), out);
  assert.deepEqual(readValues(fw.bin, findConfig(fw.bin)), values);
});

