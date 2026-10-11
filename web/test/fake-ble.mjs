// Имитация платы для Web Bluetooth: сервис DFU Adafruit в прошивке (версия 1)
// и legacy DFU загрузчика Nordic SDK 11 (версия 8). Используется в тестах и в проверке страницы.

const DFU_SERVICE = '00001530-1212-efde-1523-785feabcd123';
const CONTROL_POINT = '00001531-1212-efde-1523-785feabcd123';
const PACKET = '00001532-1212-efde-1523-785feabcd123';
const VERSION = '00001534-1212-efde-1523-785feabcd123';

function crc16(data, crc = 0xffff) {
  for (const b of data) {
    crc = ((crc >> 8) & 0xff) | ((crc << 8) & 0xff00);
    crc ^= b;
    crc ^= (crc & 0xff) >> 4;
    crc ^= (crc << 12) & 0xffff;
    crc ^= (crc & 0xff) << 5;
  }
  return crc & 0xffff;
}

const later = (fn, ms = 1) => setTimeout(fn, ms);

function domError(name, message) {
  return typeof DOMException === 'function' ? new DOMException(message, name) : Object.assign(new Error(message), { name });
}

class FakeCharacteristic extends EventTarget {
  constructor(board, uuid) {
    super();
    this.board = board;
    this.uuid = uuid;
    this.notifying = false;
  }

  async readValue() {
    if (this.uuid !== VERSION) throw domError('NotSupportedError', 'read not permitted');
    const v = new DataView(new ArrayBuffer(2));
    v.setUint16(0, this.board.mode === 'app' ? 1 : 8, true);
    return v;
  }

  async startNotifications() {
    this.notifying = true;
    return this;
  }

  notify(bytes) {
    if (!this.notifying || !this.board.connected) return;
    this.value = new DataView(new Uint8Array(bytes).buffer);
    this.dispatchEvent(new Event('characteristicvaluechanged'));
  }

  async writeValueWithResponse(bytes) {
    if (!this.board.connected) throw domError('NetworkError', 'GATT Server is disconnected');
    return this.board.onControl(this, new Uint8Array(bytes.buffer ?? bytes, bytes.byteOffset ?? 0, bytes.byteLength));
  }

  async writeValueWithoutResponse(bytes) {
    if (!this.board.connected) throw domError('NetworkError', 'GATT Server is disconnected');
    if (this.board.packetDelay) await new Promise((r) => later(r, this.board.packetDelay));
    this.board.onPacket(new Uint8Array(bytes.buffer ?? bytes, bytes.byteOffset ?? 0, bytes.byteLength).slice());
  }
}

export class FakeBleBoard {
  // mode: 'app' — прошивка в режиме обновления, 'normal' — прошивка без сервиса DFU, 'boot' — загрузчик
  // packetDelay — задержка каждого пакета данных, мс: чтобы увидеть прогресс на странице
  constructor({ mode = 'app', reconnectFailures = 0, dropPacket = -1, corruptPacket = -1, packetDelay = 0 } = {}) {
    this.mode = mode;
    this.packetDelay = packetDelay;
    this.reconnectFailures = reconnectFailures;
    this.dropPacket = dropPacket;
    this.corruptPacket = corruptPacket;
    this.connected = false;
    this.rebooting = false;
    this.state = 'idle';
    this.image = [];
    this.events = [];
    const board = this;

    this.device = Object.assign(new EventTarget(), {
      id: 'fake-imba-tv',
      get name() { return board.mode === 'boot' ? 'AdaDFU' : 'IMBA TV'; },
      gatt: {
        get connected() { return board.connected; },
        connect: () => board.connect(),
        disconnect: () => board.disconnect(false),
      },
    });
  }

  async connect() {
    await new Promise((r) => later(r, 2));
    if (this.rebooting || (this.mode === 'boot' && this.reconnectFailures-- > 0)) {
      this.events.push('connect failed');
      throw domError('NetworkError', 'Connection failed for unknown reason');
    }
    this.connected = true;
    this.events.push(`connect ${this.mode}`);
    this.cp = new FakeCharacteristic(this, CONTROL_POINT);
    this.packet = new FakeCharacteristic(this, PACKET);
    this.version = new FakeCharacteristic(this, VERSION);
    const chars = { [CONTROL_POINT]: this.cp, [PACKET]: this.packet, [VERSION]: this.version };
    return {
      connected: true,
      getPrimaryService: async (uuid) => {
        if (uuid !== DFU_SERVICE || this.mode === 'normal') throw domError('NotFoundError', 'No Services matching UUID found');
        return {
          uuid,
          getCharacteristic: async (c) => {
            if (!chars[c]) throw domError('NotFoundError', 'No Characteristics matching UUID found');
            return chars[c];
          },
        };
      },
    };
  }

  disconnect(byDevice) {
    if (!this.connected) return;
    this.connected = false;
    this.events.push(byDevice ? 'device disconnected' : 'host disconnected');
    this.device.dispatchEvent(new Event('gattserverdisconnected'));
  }

  respond(op, status = 1, delay = 1) {
    later(() => this.cp.notify([0x10, op, status]), delay);
  }

  async onControl(chr, bytes) {
    if (chr.uuid !== CONTROL_POINT) throw domError('NotSupportedError', 'write not permitted');
    // BLEDfu отвечает ошибкой CCCD, если уведомления не включены
    if (!chr.notifying) throw domError('NotSupportedError', 'GATT operation failed: CCCD improperly configured');

    if (this.mode === 'app') {
      if (bytes[0] === 0x01) {
        this.events.push('jump to bootloader');
        later(() => {
          this.disconnect(true);
          this.rebooting = true;
          later(() => { this.rebooting = false; this.mode = 'boot'; }, 30);
        }, 2);
      }
      return;
    }

    switch (bytes[0]) {
      case 0x01:
        this.state = 'size';
        break;
      case 0x02:
        if (bytes[1] === 0x00) { this.state = 'init'; this.dat = []; } else {
          this.state = 'ready';
          this.respond(0x02, this.dat.length >= 14 ? 1 : 6);
        }
        break;
      case 0x08:
        this.prn = bytes[1] | (bytes[2] << 8);
        break;
      case 0x03:
        this.state = 'data';
        this.received = 0;
        this.packets = 0;
        break;
      case 0x04: {
        const image = new Uint8Array(this.image);
        const crc = this.dat[this.dat.length - 2] | (this.dat[this.dat.length - 1] << 8);
        this.respond(0x04, image.length === this.size && crc16(image) === crc ? 1 : 5);
        break;
      }
      case 0x05:
        this.state = 'done';
        this.events.push('activate');
        later(() => this.disconnect(true), 2);
        break;
      default:
        this.respond(bytes[0], 3);
    }
  }

  onPacket(bytes) {
    if (this.state === 'size') {
      const v = new DataView(bytes.buffer);
      this.size = v.getUint32(8, true);
      this.state = 'erased';
      this.respond(0x01, 1, 5); // стирание памяти
    } else if (this.state === 'init') {
      this.dat.push(...bytes);
    } else if (this.state === 'data') {
      const n = this.packets++;
      if (n === this.dropPacket) return;
      if (n === this.corruptPacket) bytes[0] ^= 0xff;
      this.image.push(...bytes);
      this.received += bytes.length;
      if (this.prn && this.packets % this.prn === 0) {
        const note = new Uint8Array(5);
        note[0] = 0x11;
        new DataView(note.buffer).setUint32(1, this.received, true);
        later(() => this.cp.notify(note));
      }
      if (this.received >= this.size) this.respond(0x03, 1, 2);
    }
  }
}
