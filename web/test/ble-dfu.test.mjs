import { test } from 'node:test';
import assert from 'node:assert/strict';
import { BleDfu, BleDfuError, requestDfuDevice, DFU_SERVICE } from '../js/ble-dfu.js';
import { makeInitPacket } from '../js/firmware.js';
import { FakeBleBoard } from './fake-ble.mjs';

function makeImage(size) {
  const bin = new Uint8Array(size);
  for (let i = 0; i < size; i++) bin[i] = (i * 37 + (i >> 7)) & 0xff;
  return bin;
}

async function run(board, bin, { prn = 10 } = {}) {
  const log = [];
  const progress = [];
  const dfu = new BleDfu(board.device, {
    timeScale: 0.01,
    prn,
    log: (m) => log.push(m),
    onProgress: (done, total) => progress.push([done, total]),
  });
  try {
    await dfu.connect();
    if (dfu.inApplication) await dfu.enterBootloader();
    await dfu.flash(bin, makeInitPacket(bin));
  } finally {
    await dfu.disconnect();
  }
  return { log, progress };
}

test('выбор устройства: сервис DFU и загрузчик AdaDFU', async () => {
  let options;
  await requestDfuDevice({ requestDevice: async (o) => { options = o; return {}; } });
  assert.deepEqual(options.filters, [{ services: [DFU_SERVICE] }, { name: 'AdaDFU' }]);
  assert.deepEqual(options.optionalServices, [DFU_SERVICE]);
});

test('из режима обновления в прошивке: переход в загрузчик и полная прошивка', async () => {
  const board = new FakeBleBoard({ mode: 'app' });
  const bin = makeImage(1234);
  const { progress } = await run(board, bin);
  assert.equal(board.state, 'done');
  assert.deepEqual(new Uint8Array(board.image), bin);
  assert.deepEqual(progress.at(-1), [1234, 1234]);
  assert.ok(board.events.includes('jump to bootloader'));
  assert.equal(board.events.filter((e) => e === 'connect boot').length, 1);
});

test('плата уже в загрузчике', async () => {
  const board = new FakeBleBoard({ mode: 'boot' });
  const bin = makeImage(800);
  await run(board, bin);
  assert.equal(board.state, 'done');
  assert.deepEqual(new Uint8Array(board.image), bin);
  assert.ok(!board.events.includes('jump to bootloader'));
});

test('размер, кратный пакету и PRN', async () => {
  const board = new FakeBleBoard({ mode: 'boot' });
  const bin = makeImage(20 * 30);
  await run(board, bin, { prn: 10 });
  assert.deepEqual(new Uint8Array(board.image), bin);
});

test('загрузчик не сразу принимает переподключение', async () => {
  const board = new FakeBleBoard({ mode: 'app', reconnectFailures: 2 });
  const bin = makeImage(500);
  const { log } = await run(board, bin);
  assert.equal(board.state, 'done');
  assert.ok(log.some((m) => m.startsWith('Переподключение')));
});

test('прошивка без режима обновления', async () => {
  const board = new FakeBleBoard({ mode: 'normal' });
  await assert.rejects(run(board, makeImage(100)), /не включён режим обновления/);
});

test('потерянный пакет обнаруживается по подтверждению', async () => {
  const board = new FakeBleBoard({ mode: 'boot', dropPacket: 3 });
  await assert.rejects(run(board, makeImage(1000)), (e) => e instanceof BleDfuError && /Потеряны данные/.test(e.message));
  assert.notEqual(board.state, 'done');
});

test('испорченные данные не проходят проверку загрузчика', async () => {
  const board = new FakeBleBoard({ mode: 'boot', corruptPacket: 7 });
  await assert.rejects(run(board, makeImage(1000)), /ошибка контрольной суммы/);
  assert.notEqual(board.state, 'done');
});
