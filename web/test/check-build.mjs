// Проверяет, что собранные в CI файлы прошивки открываются на сайте и дают один и тот же образ.
// Использование: node web/test/check-build.mjs DIST_DIR
import { readFileSync, readdirSync } from 'node:fs';
import { join } from 'node:path';
import assert from 'node:assert/strict';
import { loadFirmware, makeUf2 } from '../js/firmware.js';
import { findConfig, readValues, applyValues, defaultValues } from '../js/config.js';

const dir = process.argv[2];
const files = readdirSync(dir);

async function load(ext) {
  const name = files.find((f) => f.endsWith(`.${ext}`));
  assert.ok(name, `нет файла .${ext} в ${dir}`);
  const data = readFileSync(join(dir, name));
  const fw = await loadFirmware(name, data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength));
  console.log(`${name}: образ ${fw.bin.length} байт, init-пакет ${Buffer.from(fw.dat).toString('hex')}`);
  return fw;
}

const zip = await load('zip');
const hex = await load('hex');
const uf2 = await load('uf2');

assert.deepEqual(hex.bin, zip.bin, 'образ из .hex отличается от DFU-пакета');
assert.deepEqual(uf2.bin.subarray(0, zip.bin.length), zip.bin, 'образ из .uf2 отличается от DFU-пакета');
assert.ok(uf2.bin.subarray(zip.bin.length).every((b) => b === 0xff), 'лишние данные в конце .uf2');
assert.deepEqual(makeUf2(zip.bin), new Uint8Array(readFileSync(join(dir, files.find((f) => f.endsWith('.uf2'))))),
  '.uf2 из браузера отличается от .uf2 из CI');

// Блок настроек: сайт должен найти его и прочитать значения по умолчанию из config.json
const source = JSON.parse(readFileSync(new URL('../../IMBA_TV/config.json', import.meta.url), 'utf8'));
const cfg = findConfig(zip.bin);
assert.ok(cfg, 'в прошивке нет блока настроек');
const strip = (s) => s.groups.map((g) => ({ ...g, fields: g.fields.map(({ offset, size, ...f }) => f) }));
assert.deepEqual(strip(cfg.schema), strip(source), 'описание настроек в прошивке отличается от config.json');
assert.deepEqual(readValues(zip.bin, cfg), defaultValues(cfg), 'значения в прошивке отличаются от config.json');
assert.equal(findConfig(applyValues(zip.bin, cfg, defaultValues(cfg))).offset, cfg.offset);
console.log(`Блок настроек: ${cfg.fields.length} полей, ${cfg.size} байт по адресу 0x${(0x26000 + cfg.offset).toString(16)}`);

console.log('Все файлы прошивки читаются сайтом и совпадают');
