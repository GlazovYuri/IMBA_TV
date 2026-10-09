import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { loadFirmware, readZip, crc16, FirmwareError } from '../js/firmware.js';

const fixture = (name) => readFileSync(new URL(`fixtures/${name}`, import.meta.url));
const buf = (b) => b.buffer.slice(b.byteOffset, b.byteOffset + b.byteLength);

// Эталон — пакет, собранный adafruit-nrfutil из app.hex (см. fixtures/generate.py)
const reference = await readZip(buf(fixture('app.zip')));
const refBin = reference.get('app.bin');
const refDat = reference.get('app.dat');

test('crc16 совпадает с nrfutil', () => {
  assert.equal(crc16(refBin), refDat[refDat.length - 2] | (refDat[refDat.length - 1] << 8));
});

test('DFU-пакет из Arduino/nrfutil', async () => {
  const fw = await loadFirmware('app.zip', buf(fixture('app.zip')));
  assert.equal(fw.format, 'zip');
  assert.deepEqual(fw.bin, refBin);
  assert.deepEqual(fw.dat, refDat);
});

test('сжатый zip с вложенной папкой', async () => {
  const fw = await loadFirmware('firmware.zip', buf(fixture('app-deflate.zip')));
  assert.deepEqual(fw.bin, refBin);
  assert.deepEqual(fw.dat, refDat);
});

test('HEX даёт тот же образ и init-пакет, что nrfutil', async () => {
  const fw = await loadFirmware('app.hex', buf(fixture('app.hex')));
  assert.equal(fw.format, 'hex');
  assert.deepEqual(fw.bin, refBin);
  assert.deepEqual(fw.dat, refDat);
});

test('UF2 даёт тот же образ, дополненный 0xFF до страницы', async () => {
  const fw = await loadFirmware('app.uf2', buf(fixture('app.uf2')));
  assert.equal(fw.format, 'uf2');
  assert.deepEqual(fw.bin.subarray(0, refBin.length), refBin);
  assert.ok(fw.bin.subarray(refBin.length).every((b) => b === 0xff));
  assert.equal(crc16(fw.bin), fw.dat[fw.dat.length - 2] | (fw.dat[fw.dat.length - 1] << 8));
});

test('повреждённый образ в zip отклоняется', async () => {
  const zip = new Uint8Array(fixture('app.zip'));
  const at = Buffer.from(zip).indexOf(Buffer.from(refBin.subarray(100, 132)));
  assert.ok(at > 0);
  zip[at] ^= 0xff;
  await assert.rejects(loadFirmware('app.zip', zip.buffer), /повреждён/);
});

test('HEX с другим начальным адресом отклоняется', async () => {
  const hex = fixture('app.hex').toString()
    .replace(':020000040002F8', ':020000040003F7'); // 0x2xxxx → 0x3xxxx
  await assert.rejects(loadFirmware('app.hex', new TextEncoder().encode(hex).buffer), /0x36000/);
});

test('посторонние файлы отклоняются', async () => {
  await assert.rejects(loadFirmware('photo.jpg', new Uint8Array([0xff, 0xd8, 0xff]).buffer), FirmwareError);
  await assert.rejects(loadFirmware('notes.hex', new TextEncoder().encode(':zz\n').buffer), /HEX/);
});
