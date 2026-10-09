// Serial DFU загрузчика Adafruit nRF52 (Nordic SDK 11, HCI-пакеты поверх SLIP) через Web Serial.
// Повторяет то, что делает `adafruit-nrfutil dfu serial` при загрузке из Arduino IDE:
// start → init → данные кусками по 512 байт → stop, с теми же паузами на стирание и запись флеш-памяти.

import { crc16 } from './firmware.js';

const DFU_INIT_PACKET = 1;
const DFU_START_PACKET = 3;
const DFU_DATA_PACKET = 4;
const DFU_STOP_DATA_PACKET = 5;
const DFU_UPDATE_MODE_APP = 4;

const HCI_PACKET_TYPE = 14;
const DFU_PACKET_MAX_SIZE = 512;

const FLASH_PAGE_SIZE = 4096;
const FLASH_PAGE_ERASE_TIME = 89.7; // мс
const FLASH_PAGE_WRITE_TIME = (FLASH_PAGE_SIZE / 4) * 0.1; // мс

const ACK_TIMEOUT = 1000; // мс
const MAX_ATTEMPTS = 4;

const SLIP_END = 0xc0;
const SLIP_ESC = 0xdb;
const SLIP_ESC_END = 0xdc;
const SLIP_ESC_ESC = 0xdd;

export const BAUD_RATE = 115200;
export const TOUCH_BAUD_RATE = 1200;

export class DfuError extends Error {}

function u32(value) {
  const out = new Uint8Array(4);
  new DataView(out.buffer).setUint32(0, value >>> 0, true);
  return out;
}

function concat(...parts) {
  const out = new Uint8Array(parts.reduce((n, p) => n + p.length, 0));
  let off = 0;
  for (const p of parts) { out.set(p, off); off += p.length; }
  return out;
}

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

// Заголовок HCI: seq, ack = seq+1, флаги «есть CRC» и «надёжный пакет», тип, длина, контрольная сумма
export function hciPacket(seq, payload) {
  const len = payload.length;
  const h0 = seq | (((seq + 1) % 8) << 3) | (1 << 6) | (1 << 7);
  const h1 = HCI_PACKET_TYPE | ((len & 0x0f) << 4);
  const h2 = (len & 0xff0) >> 4;
  const h3 = (~(h0 + h1 + h2) + 1) & 0xff;

  const body = concat(new Uint8Array([h0, h1, h2, h3]), payload);
  const crc = crc16(body);
  const raw = concat(body, new Uint8Array([crc & 0xff, crc >> 8]));

  const out = [SLIP_END];
  for (const b of raw) {
    if (b === SLIP_END) out.push(SLIP_ESC, SLIP_ESC_END);
    else if (b === SLIP_ESC) out.push(SLIP_ESC, SLIP_ESC_ESC);
    else out.push(b);
  }
  out.push(SLIP_END);
  return new Uint8Array(out);
}

// Собирает SLIP-кадры из потока байт
export class SlipDecoder {
  constructor(onFrame) {
    this.onFrame = onFrame;
    this.frame = null;
    this.escape = false;
  }

  push(chunk) {
    for (const b of chunk) {
      if (b === SLIP_END) {
        if (this.frame && this.frame.length) this.onFrame(new Uint8Array(this.frame));
        this.frame = [];
        this.escape = false;
      } else if (this.frame) {
        if (this.escape) {
          this.frame.push(b === SLIP_ESC_END ? SLIP_END : b === SLIP_ESC_ESC ? SLIP_ESC : b);
          this.escape = false;
        } else if (b === SLIP_ESC) {
          this.escape = true;
        } else {
          this.frame.push(b);
        }
      }
    }
  }
}

// Номер подтверждения из ACK-пакета загрузчика или null, если кадр не ACK
export function parseAck(frame) {
  if (frame.length !== 4) return null;
  if (((frame[0] + frame[1] + frame[2] + frame[3]) & 0xff) !== 0) return null;
  return (frame[0] >> 3) & 0x07;
}

// Время стирания и переноса образа — как в nrfutil (dfu_transport_serial.py)
export function eraseWaitTime(size) {
  return Math.max(500, (Math.floor(size / FLASH_PAGE_SIZE) + 1) * FLASH_PAGE_ERASE_TIME);
}

export function activateWaitTime(size) {
  return eraseWaitTime(size) + (Math.floor(size / FLASH_PAGE_SIZE) + 1) * FLASH_PAGE_WRITE_TIME;
}

// 1200 бод + сброс DTR: прошивка на ядре Adafruit перезагружается в режим загрузчика
export async function touch1200(port) {
  await port.open({ baudRate: TOUCH_BAUD_RATE });
  try {
    await port.setSignals({ dataTerminalReady: true });
    await sleep(50);
    await port.setSignals({ dataTerminalReady: false });
  } catch {
    // плата могла уже начать перезагрузку
  }
  try {
    await port.close();
  } catch {
    // порт пропал вместе с перезагрузкой — это нормально
  }
}

export class SerialDfu {
  // timeScale позволяет ускорить паузы в тестах; в браузере всегда 1
  constructor(port, { log = () => {}, onProgress = () => {}, timeScale = 1 } = {}) {
    this.port = port;
    this.log = log;
    this.onProgress = onProgress;
    this.timeScale = timeScale;
    this.seq = 0;
    this.frames = [];
    this.waiter = null;
    this.readError = null;
  }

  wait(ms) {
    return sleep(ms * this.timeScale);
  }

  async open() {
    await this.port.open({ baudRate: BAUD_RATE });
    this.writer = this.port.writable.getWriter();
    this.reader = this.port.readable.getReader();
    const decoder = new SlipDecoder((frame) => this.onFrame(frame));
    this.readLoop = (async () => {
      try {
        for (;;) {
          const { value, done } = await this.reader.read();
          if (done) break;
          if (value) decoder.push(value);
        }
      } catch (e) {
        this.readError = e;
      }
      this.wakeWaiter();
    })();
  }

  async close() {
    try { await this.reader?.cancel(); } catch { /* порт уже закрыт */ }
    await this.readLoop;
    try { this.reader?.releaseLock(); } catch { /* ignore */ }
    try { this.writer?.releaseLock(); } catch { /* ignore */ }
    try { await this.port.close(); } catch { /* плата могла уже перезагрузиться */ }
  }

  onFrame(frame) {
    this.frames.push(frame);
    this.wakeWaiter();
  }

  wakeWaiter() {
    const w = this.waiter;
    this.waiter = null;
    w?.();
  }

  // Ждёт ACK от загрузчика; null — если не дождались
  async waitAck(timeout) {
    const deadline = Date.now() + timeout;
    for (;;) {
      while (this.frames.length) {
        const ack = parseAck(this.frames.shift());
        if (ack !== null) return ack;
      }
      if (this.readError) throw new DfuError(`Связь с платой потеряна: ${this.readError.message}`);
      const left = deadline - Date.now();
      if (left <= 0) return null;
      await new Promise((resolve) => {
        const t = setTimeout(resolve, left);
        this.waiter = () => { clearTimeout(t); resolve(); };
      });
    }
  }

  // Отправка с подтверждением. Повтор с тем же номером безопасен: загрузчик
  // отбрасывает пакет с неожиданным номером и в ACK сообщает, какой номер ждёт.
  async sendPacket(payload) {
    this.seq = (this.seq + 1) % 8;
    for (let attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
      this.frames = [];
      await this.writer.write(hciPacket(this.seq, payload));

      const expected = (this.seq + 1) % 8;
      const deadline = Date.now() + ACK_TIMEOUT;
      let ack;
      // ACK с номером текущего пакета — запоздалый ответ на повтор предыдущего, пропускаем
      do {
        ack = await this.waitAck(Math.max(0, deadline - Date.now()));
      } while (ack === this.seq);

      if (ack === expected) return;
      if (ack === null) {
        this.log(`Нет ответа на пакет #${this.seq}, повтор (${attempt}/${MAX_ATTEMPTS})`);
      } else {
        this.log(`Загрузчик ждёт пакет #${ack}, синхронизация`);
        this.seq = ack;
      }
    }
    throw new DfuError('Загрузчик не отвечает');
  }

  async flash(bin, dat) {
    const chunks = Math.ceil(bin.length / DFU_PACKET_MAX_SIZE);
    this.onProgress(0, bin.length);

    this.log(`Старт DFU, размер приложения ${bin.length} байт`);
    await this.sendPacket(concat(u32(DFU_START_PACKET), u32(DFU_UPDATE_MODE_APP), u32(0), u32(0), u32(bin.length)));
    await this.wait(eraseWaitTime(bin.length));

    this.log('Отправка init-пакета');
    await this.sendPacket(concat(u32(DFU_INIT_PACKET), dat, new Uint8Array(2)));

    this.log(`Отправка прошивки: ${chunks} пакетов`);
    for (let i = 0; i < chunks; i++) {
      const part = bin.subarray(i * DFU_PACKET_MAX_SIZE, (i + 1) * DFU_PACKET_MAX_SIZE);
      await this.sendPacket(concat(u32(DFU_DATA_PACKET), part));
      this.onProgress(Math.min(bin.length, (i + 1) * DFU_PACKET_MAX_SIZE), bin.length);
      // каждые 4 КБ загрузчик пишет страницу флеш-памяти и не отвечает
      if (i % 8 === 0) await this.wait(FLASH_PAGE_WRITE_TIME);
    }
    await this.wait(FLASH_PAGE_WRITE_TIME);

    this.log('Завершение передачи');
    try {
      await this.sendPacket(u32(DFU_STOP_DATA_PACKET));
    } catch (e) {
      // все данные уже подтверждены; загрузчик мог перезагрузиться раньше, чем отправил ACK
      this.log(`Нет подтверждения завершения: ${e.message}`);
    }
  }
}
