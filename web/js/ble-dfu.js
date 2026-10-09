// Обновление по Bluetooth через Web Bluetooth: legacy DFU Nordic SDK 11,
// который поддерживает загрузчик Adafruit nRF52 (так же работают nRF Connect и Bluefruit Connect).
//
// 1. В прошивке (режим обновления) работает сервис DFU Adafruit (BLEDfu) с версией 1.
//    Команда START перезагружает плату в загрузчик, тот ждёт переподключения этого же телефона.
// 2. В загрузчике (версия сервиса не 1): START + размеры, init-пакет, данные пакетами
//    по 20 байт с подтверждением каждые PRN пакетов, проверка и активация.

export const DFU_SERVICE = '00001530-1212-efde-1523-785feabcd123';
const DFU_CONTROL_POINT = '00001531-1212-efde-1523-785feabcd123';
const DFU_PACKET = '00001532-1212-efde-1523-785feabcd123';
const DFU_VERSION = '00001534-1212-efde-1523-785feabcd123';

const OP_START = 0x01;
const OP_INIT = 0x02;
const OP_RECEIVE = 0x03;
const OP_VALIDATE = 0x04;
const OP_ACTIVATE = 0x05;
const OP_PRN_REQUEST = 0x08;
const OP_RESPONSE = 0x10;
const OP_PRN = 0x11;
const UPDATE_APPLICATION = 0x04;
const APP_MODE_VERSION = 1;

const PACKET_SIZE = 20;
const STATUS = {
  2: 'загрузчик в неверном состоянии',
  3: 'команда не поддерживается',
  4: 'прошивка слишком большая',
  5: 'ошибка контрольной суммы',
  6: 'ошибка записи',
};

export class BleDfuError extends Error {}

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

function withTimeout(promise, ms, message) {
  let timer;
  return Promise.race([
    promise.finally(() => clearTimeout(timer)),
    new Promise((_, reject) => { timer = setTimeout(() => reject(new BleDfuError(message)), ms); }),
  ]);
}

function u32le(...values) {
  const out = new Uint8Array(values.length * 4);
  const v = new DataView(out.buffer);
  values.forEach((n, i) => v.setUint32(i * 4, n, true));
  return out;
}

export function requestDfuDevice(bluetooth = navigator.bluetooth) {
  return bluetooth.requestDevice({
    // плата в режиме обновления рекламирует сервис DFU, загрузчик называется AdaDFU
    filters: [{ services: [DFU_SERVICE] }, { name: 'AdaDFU' }],
    optionalServices: [DFU_SERVICE],
  });
}

export class BleDfu {
  // timeScale ускоряет паузы в тестах
  constructor(device, { log = () => {}, onProgress = () => {}, timeScale = 1, prn = 10 } = {}) {
    this.device = device;
    this.log = log;
    this.onProgress = onProgress;
    this.timeScale = timeScale;
    this.prn = prn;
    this.notes = [];
    this.waiter = null;
    this.onNotify = (e) => {
      const v = e.target.value;
      this.notes.push(new Uint8Array(v.buffer, v.byteOffset, v.byteLength).slice());
      this.wake();
    };
    this.onDisconnect = () => { this.disconnected = true; this.wake(); };
    device.addEventListener('gattserverdisconnected', this.onDisconnect);
  }

  wait(ms) {
    return sleep(ms * this.timeScale);
  }

  wake() {
    const w = this.waiter;
    this.waiter = null;
    w?.();
  }

  // Подключение и поиск сервиса DFU; version === 1 — плата ещё в прошивке, а не в загрузчике
  async connect() {
    this.disconnected = false;
    this.notes = [];
    const server = await withTimeout(this.device.gatt.connect(), 20000 * this.timeScale, 'Не удалось подключиться к плате');
    let service;
    try {
      service = await server.getPrimaryService(DFU_SERVICE);
    } catch {
      throw Object.assign(new BleDfuError('На плате не включён режим обновления по Bluetooth'), { code: 'no-dfu' });
    }
    this.controlPoint = await service.getCharacteristic(DFU_CONTROL_POINT);
    this.packet = await service.getCharacteristic(DFU_PACKET);
    try {
      const value = await (await service.getCharacteristic(DFU_VERSION)).readValue();
      this.version = value.getUint16(0, true);
    } catch {
      this.version = null;
    }
    this.controlPoint.addEventListener('characteristicvaluechanged', this.onNotify);
    await this.controlPoint.startNotifications();
    this.log(`Подключено к ${this.device.name ?? 'плате'}, версия DFU ${this.version ?? 'неизвестна'}`);
  }

  get inApplication() {
    return this.version === APP_MODE_VERSION;
  }

  async disconnect() {
    this.device.removeEventListener('gattserverdisconnected', this.onDisconnect);
    try {
      if (this.device.gatt.connected) this.device.gatt.disconnect();
    } catch {
      // уже отключено
    }
  }

  writeControl(bytes) {
    const c = this.controlPoint;
    return c.writeValueWithResponse ? c.writeValueWithResponse(bytes) : c.writeValue(bytes);
  }

  writePacket(bytes) {
    const c = this.packet;
    return c.writeValueWithoutResponse ? c.writeValueWithoutResponse(bytes) : c.writeValue(bytes);
  }

  // Ждёт уведомление, подходящее под match. Ответ загрузчика с ошибкой прерывает передачу
  async waitNote(match, timeout, what) {
    const deadline = Date.now() + timeout * this.timeScale;
    for (;;) {
      while (this.notes.length) {
        const note = this.notes.shift();
        if (note[0] === OP_RESPONSE && note[2] !== 1) {
          throw new BleDfuError(`Загрузчик отклонил команду ${note[1]}: ${STATUS[note[2]] ?? `код ${note[2]}`}`);
        }
        if (match(note)) return note;
      }
      if (this.disconnected) throw new BleDfuError(`Плата отключилась (${what})`);
      const left = deadline - Date.now();
      if (left <= 0) throw new BleDfuError(`Плата не ответила: ${what}`);
      await new Promise((resolve) => {
        const t = setTimeout(resolve, left);
        this.waiter = () => { clearTimeout(t); resolve(); };
      });
    }
  }

  waitResponse(op, timeout, what) {
    return this.waitNote((n) => n[0] === OP_RESPONSE && n[1] === op, timeout, what);
  }

  // Плата в режиме обновления: START перезагружает её в загрузчик, затем переподключаемся
  async enterBootloader() {
    this.log('Перезагрузка платы в загрузчик');
    try {
      await this.writeControl(new Uint8Array([OP_START, UPDATE_APPLICATION]));
    } catch {
      // плата может отключиться, не дождавшись подтверждения записи
    }
    const deadline = Date.now() + 10000 * this.timeScale;
    while (!this.disconnected && Date.now() < deadline) await this.wait(100);

    // загрузчик около минуты ждёт переподключения именно этого телефона
    for (let attempt = 1; attempt <= 6; attempt++) {
      await this.wait(1500);
      try {
        await this.connect();
        if (!this.inApplication) return;
        throw new BleDfuError('Плата не перешла в загрузчик');
      } catch (e) {
        this.log(`Переподключение ${attempt}/6: ${e.message}`);
      }
    }
    throw new BleDfuError('Плата перезагрузилась в режим обновления, но переподключиться не удалось');
  }

  async flash(bin, dat) {
    const packets = Math.ceil(bin.length / PACKET_SIZE);
    this.notes = [];
    this.onProgress(0, bin.length);

    this.log(`Старт DFU, размер приложения ${bin.length} байт`);
    await this.writeControl(new Uint8Array([OP_START, UPDATE_APPLICATION]));
    await this.writePacket(u32le(0, 0, bin.length));
    // загрузчик стирает место под прошивку и только потом отвечает
    await this.waitResponse(OP_START, 60000, 'стирание памяти');

    this.log('Отправка init-пакета');
    await this.writeControl(new Uint8Array([OP_INIT, 0x00]));
    for (let i = 0; i < dat.length; i += PACKET_SIZE) await this.writePacket(dat.subarray(i, i + PACKET_SIZE));
    await this.writeControl(new Uint8Array([OP_INIT, 0x01]));
    await this.waitResponse(OP_INIT, 10000, 'init-пакет');

    await this.writeControl(new Uint8Array([OP_PRN_REQUEST, this.prn & 0xff, this.prn >> 8]));
    await this.writeControl(new Uint8Array([OP_RECEIVE]));

    this.log(`Отправка прошивки: ${packets} пакетов по ${PACKET_SIZE} байт`);
    for (let i = 0; i < packets; i++) {
      await this.writePacket(bin.subarray(i * PACKET_SIZE, (i + 1) * PACKET_SIZE));
      const sent = Math.min(bin.length, (i + 1) * PACKET_SIZE);
      if ((i + 1) % this.prn === 0 && i + 1 < packets) {
        // загрузчик сообщает, сколько байт принял: так не переполняем его буфер и замечаем потери
        const note = await this.waitNote((n) => n[0] === OP_PRN, 10000, 'подтверждение пакетов');
        const received = new DataView(note.buffer).getUint32(1, true);
        if (received !== sent) throw new BleDfuError(`Потеряны данные: отправлено ${sent} байт, принято ${received}`);
      }
      this.onProgress(sent, bin.length);
    }
    await this.waitResponse(OP_RECEIVE, 30000, 'приём прошивки');

    this.log('Проверка прошивки');
    await this.writeControl(new Uint8Array([OP_VALIDATE]));
    await this.waitResponse(OP_VALIDATE, 30000, 'проверка прошивки');

    this.log('Активация прошивки');
    try {
      await this.writeControl(new Uint8Array([OP_ACTIVATE]));
    } catch {
      // загрузчик перезагружается сразу после команды
    }
  }
}
