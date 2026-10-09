// Проверяет, что собранные в CI файлы прошивки открываются на сайте и дают один и тот же образ.
// Использование: node web/test/check-build.mjs DIST_DIR
import { readFileSync, readdirSync } from 'node:fs';
import { join } from 'node:path';
import assert from 'node:assert/strict';
import { loadFirmware } from '../js/firmware.js';

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
console.log('Все файлы прошивки читаются сайтом и совпадают');
