import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { hciPacket, SlipDecoder, SerialDfu } from '../js/dfu.js';
import { crc16, makeInitPacket } from '../js/firmware.js';

const golden = JSON.parse(readFileSync(new URL('fixtures/hci_frames.json', import.meta.url)));

test('HCI-кадры побайтно совпадают с adafruit-nrfutil', () => {
  golden.forEach(({ payload, frame }, i) => {
    assert.deepEqual([...hciPacket(i + 1, new Uint8Array(payload))], frame);
  });
});

test('SLIP-декодер восстанавливает кадр', () => {
  const frames = [];
  const decoder = new SlipDecoder((f) => frames.push(f));
  const packet = hciPacket(2, new Uint8Array(golden[1].payload));
  // кадр приходит кусками произвольной длины
  for (let i = 0; i < packet.length; i += 7) decoder.push(packet.subarray(i, i + 7));
  assert.equal(frames.length, 1);
  assert.deepEqual([...frames[0].subarray(4, -2)], golden[1].payload);
});

// Загрузчик со стороны платы: HCI-транспорт и DFU из Nordic SDK 11, как в Adafruit nRF52 Bootloader
class FakeBootloader {
  constructor({ expectedSeq = 1, drop = [], duplicateAck = [] } = {}) {
    this.expectedSeq = expectedSeq;
    this.drop = new Set(drop);
    this.duplicateAck = new Set(duplicateAck);
    this.received = 0;
    this.state = 'idle';
    this.image = [];
    this.isOpen = false;

    const decoder = new SlipDecoder((frame) => this.onFrame(frame));
    this.writable = new WritableStream({ write: (chunk) => decoder.push(chunk) });
    this.readable = new ReadableStream({ start: (c) => { this.out = c; } });
  }

  async open() { this.isOpen = true; }
  async close() { this.isOpen = false; }

  ack() {
    const h = [this.expectedSeq << 3, 0, 0];
    h.push((~(h[0] + h[1] + h[2]) + 1) & 0xff);
    const frame = new Uint8Array([0xc0, ...h, 0xc0]);
    setTimeout(() => this.out.enqueue(frame), 1);
  }

  onFrame(frame) {
    const n = this.received++;
    if (this.drop.has(n)) return; // пакет потерян по дороге

    const len = (frame[1] >> 4) | (frame[2] << 4);
    if (((frame[0] + frame[1] + frame[2] + frame[3]) & 0xff) !== 0) return;
    if (frame.length !== len + 6) return;
    const crc = frame[len + 4] | (frame[len + 5] << 8);
    if (crc16(frame.subarray(0, len + 4)) !== crc) return;

    const seq = frame[0] & 7;
    if (seq !== this.expectedSeq) { this.ack(); return; }
    this.expectedSeq = (this.expectedSeq + 1) % 8;
    this.ack();
    if (this.duplicateAck.has(n)) this.ack();
    this.process(frame.subarray(4, 4 + len));
  }

  process(p) {
    const v = new DataView(p.buffer, p.byteOffset, p.byteLength);
    switch (v.getUint32(0, true)) {
      case 3:
        assert.equal(this.state, 'idle');
        assert.equal(v.getUint32(4, true), 4); // только приложение
        this.appSize = v.getUint32(16, true);
        this.state = 'started';
        break;
      case 1:
        assert.equal(this.state, 'started');
        this.dat = p.slice(4, p.length - 2);
        this.state = 'init';
        break;
      case 4:
        assert.equal(this.state, 'init');
        this.image.push(...p.subarray(4));
        break;
      case 5: {
        const image = new Uint8Array(this.image);
        assert.equal(image.length, this.appSize);
        assert.equal(crc16(image), this.dat[this.dat.length - 2] | (this.dat[this.dat.length - 1] << 8));
        this.state = 'done';
        break;
      }
      default:
        assert.fail('неизвестный DFU-пакет');
    }
  }
}

function makeImage(size) {
  const bin = new Uint8Array(size);
  for (let i = 0; i < size; i++) bin[i] = (i * 31 + (i >> 8)) & 0xff;
  return bin;
}

async function flash(device, bin) {
  const log = [];
  const progress = [];
  const dfu = new SerialDfu(device, {
    timeScale: 0.01,
    log: (m) => log.push(m),
    onProgress: (done, total) => progress.push([done, total]),
  });
  await dfu.open();
  try {
    await dfu.flash(bin, makeInitPacket(bin));
  } finally {
    await dfu.close();
  }
  return { log, progress };
}

test('прошивка доходит до загрузчика целиком', async () => {
  const device = new FakeBootloader();
  const bin = makeImage(5000);
  const { progress } = await flash(device, bin);
  assert.equal(device.state, 'done');
  assert.deepEqual(new Uint8Array(device.image), bin);
  assert.deepEqual(progress.at(-1), [5000, 5000]);
  assert.equal(device.isOpen, false);
});

test('потерянный пакет отправляется повторно', async () => {
  const device = new FakeBootloader({ drop: [3] });
  const bin = makeImage(3000);
  const { log } = await flash(device, bin);
  assert.equal(device.state, 'done');
  assert.deepEqual(new Uint8Array(device.image), bin);
  assert.ok(log.some((m) => m.includes('повтор')));
});

test('запоздалый повторный ACK не сбивает передачу', async () => {
  const device = new FakeBootloader({ duplicateAck: [2, 5] });
  const bin = makeImage(4000);
  const { log } = await flash(device, bin);
  assert.equal(device.state, 'done');
  assert.deepEqual(new Uint8Array(device.image), bin);
  assert.deepEqual(log.filter((m) => m.includes('синхронизация') || m.includes('повтор')), []);
});

test('синхронизация номера пакета с загрузчиком', async () => {
  const device = new FakeBootloader({ expectedSeq: 5 });
  const bin = makeImage(1500);
  const { log } = await flash(device, bin);
  assert.equal(device.state, 'done');
  assert.deepEqual(new Uint8Array(device.image), bin);
  assert.ok(log.some((m) => m.includes('синхронизация')));
});
