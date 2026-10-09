// Разбор файлов прошивки: DFU-пакет (.zip), Intel HEX (.hex) и UF2 (.uf2).
// Результат всегда один: образ приложения (bin) и init-пакет (dat) для serial DFU
// загрузчика Adafruit nRF52 — те же данные, что Arduino IDE передаёт через adafruit-nrfutil.

// Приложение для SoftDevice S140 6.1.1 начинается с этого адреса (nrf52840_s140_v6.ld)
export const APP_START = 0x26000;
// upload.maximum_size из boards/boards.txt
export const APP_MAX_SIZE = 815104;

const UF2_FAMILY_NRF52840 = 0xada52840;
const UICR_START = 0x10000000;
const MBR_END = 0x1000;

// Параметры init-пакета, как в recipe.objcopy.zip.pattern ядра Adafruit nRF52
const DEVICE_TYPE = 0x0052;
const DEVICE_REVISION = 0xffff;
const APP_VERSION = 0xffffffff;
const SD_REQ_ANY = 0xfffe;

export class FirmwareError extends Error {}

// CRC16-CCITT в варианте Nordic (crc16_compute), начальное значение 0xFFFF
export function crc16(data, crc = 0xffff) {
  for (const b of data) {
    crc = ((crc >> 8) & 0xff) | ((crc << 8) & 0xff00);
    crc ^= b;
    crc ^= (crc & 0xff) >> 4;
    crc ^= (crc << 12) & 0xffff;
    crc ^= (crc & 0xff) << 5;
  }
  return crc & 0xffff;
}

// Init-пакет DFU 0.5: dev_type, dev_rev, app_version, список SoftDevice, CRC16 образа
export function makeInitPacket(bin, { sdReq = [SD_REQ_ANY] } = {}) {
  const out = new Uint8Array(10 + sdReq.length * 2 + 2);
  const v = new DataView(out.buffer);
  v.setUint16(0, DEVICE_TYPE, true);
  v.setUint16(2, DEVICE_REVISION, true);
  v.setUint32(4, APP_VERSION, true);
  v.setUint16(8, sdReq.length, true);
  sdReq.forEach((sd, i) => v.setUint16(10 + i * 2, sd, true));
  v.setUint16(10 + sdReq.length * 2, crc16(bin), true);
  return out;
}

// Тот же init-пакет, но с CRC изменённого образа (например, после записи настроек)
export function withImageCrc(dat, bin) {
  const out = dat.slice();
  const crc = crc16(bin);
  out[out.length - 2] = crc & 0xff;
  out[out.length - 1] = crc >> 8;
  return out;
}

// UF2 для копирования на диск загрузчика, как .github/scripts/hex2uf2.py
export function makeUf2(bin, start = APP_START) {
  const blocks = Math.ceil(bin.length / 256);
  const out = new Uint8Array(blocks * 512);
  const v = new DataView(out.buffer);
  for (let n = 0; n < blocks; n++) {
    const off = n * 512;
    [0x0a324655, 0x9e5d5157, 0x2000, start + n * 256, 256, n, blocks, UF2_FAMILY_NRF52840]
      .forEach((word, i) => v.setUint32(off + i * 4, word, true));
    out.fill(0xff, off + 32, off + 32 + 256);
    out.set(bin.subarray(n * 256, (n + 1) * 256), off + 32);
    v.setUint32(off + 508, 0x0ab16f30, true);
  }
  return out;
}

// ---------- ZIP ----------

async function inflateRaw(data) {
  if (typeof DecompressionStream === 'undefined') {
    throw new FirmwareError('Браузер не умеет распаковывать сжатые zip-архивы');
  }
  const stream = new Blob([data]).stream().pipeThrough(new DecompressionStream('deflate-raw'));
  return new Uint8Array(await new Response(stream).arrayBuffer());
}

export async function readZip(buffer) {
  const bytes = new Uint8Array(buffer);
  const v = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);

  // End of central directory ищем с конца (после него может идти комментарий до 64 КБ)
  let eocd = -1;
  for (let i = bytes.length - 22; i >= Math.max(0, bytes.length - 22 - 0xffff); i--) {
    if (v.getUint32(i, true) === 0x06054b50) { eocd = i; break; }
  }
  if (eocd < 0) throw new FirmwareError('Файл не похож на zip-архив');

  const count = v.getUint16(eocd + 10, true);
  let p = v.getUint32(eocd + 16, true);
  const files = new Map();
  const decoder = new TextDecoder();

  for (let n = 0; n < count; n++) {
    if (p + 46 > bytes.length || v.getUint32(p, true) !== 0x02014b50) {
      throw new FirmwareError('Повреждённый zip-архив');
    }
    const method = v.getUint16(p + 10, true);
    const compSize = v.getUint32(p + 20, true);
    const nameLen = v.getUint16(p + 28, true);
    const extraLen = v.getUint16(p + 30, true);
    const commentLen = v.getUint16(p + 32, true);
    const local = v.getUint32(p + 42, true);
    const name = decoder.decode(bytes.subarray(p + 46, p + 46 + nameLen));
    p += 46 + nameLen + extraLen + commentLen;

    if (name.endsWith('/')) continue;
    if (local + 30 > bytes.length || v.getUint32(local, true) !== 0x04034b50) {
      throw new FirmwareError('Повреждённый zip-архив');
    }
    const start = local + 30 + v.getUint16(local + 26, true) + v.getUint16(local + 28, true);
    const raw = bytes.subarray(start, start + compSize);
    if (method === 0) files.set(name, raw.slice());
    else if (method === 8) files.set(name, await inflateRaw(raw));
    else throw new FirmwareError(`Неподдерживаемый метод сжатия в zip (${method})`);
  }
  return files;
}

function baseName(path) {
  return path.split('/').pop();
}

function findFile(files, name) {
  if (files.has(name)) return files.get(name);
  for (const [path, data] of files) if (baseName(path) === baseName(name)) return data;
  return null;
}

export async function parseDfuZip(buffer) {
  const files = await readZip(buffer);
  const manifestRaw = findFile(files, 'manifest.json');
  if (!manifestRaw) throw new FirmwareError('В архиве нет manifest.json — это не DFU-пакет');

  let manifest;
  try {
    manifest = JSON.parse(new TextDecoder().decode(manifestRaw)).manifest;
  } catch {
    throw new FirmwareError('Не удалось прочитать manifest.json');
  }
  if (!manifest) throw new FirmwareError('Не удалось прочитать manifest.json');
  if (manifest.softdevice || manifest.bootloader || manifest.softdevice_bootloader) {
    throw new FirmwareError('Пакет обновляет SoftDevice или загрузчик — через сайт можно прошить только приложение');
  }
  const app = manifest.application;
  if (!app) throw new FirmwareError('В DFU-пакете нет прошивки приложения');

  const bin = findFile(files, app.bin_file);
  const dat = findFile(files, app.dat_file);
  if (!bin || !dat) throw new FirmwareError('В архиве не хватает файлов из manifest.json');
  return { bin, dat };
}

// ---------- Intel HEX ----------

export function parseIntelHex(text) {
  const chunks = [];
  let base = 0;
  let ended = false;
  const lines = text.split(/\r?\n/);

  for (let ln = 0; ln < lines.length && !ended; ln++) {
    const line = lines[ln].trim();
    if (!line) continue;
    if (line[0] !== ':' || line.length < 11 || line.length % 2 === 0 || /[^0-9a-f]/i.test(line.slice(1))) {
      throw new FirmwareError(`Ошибка в HEX-файле, строка ${ln + 1}`);
    }
    const rec = new Uint8Array((line.length - 1) / 2);
    for (let i = 0; i < rec.length; i++) rec[i] = parseInt(line.substr(1 + i * 2, 2), 16);

    const len = rec[0];
    if (rec.length !== len + 5) throw new FirmwareError(`Ошибка в HEX-файле, строка ${ln + 1}`);
    let sum = 0;
    for (const b of rec) sum += b;
    if (sum & 0xff) throw new FirmwareError(`Неверная контрольная сумма в HEX-файле, строка ${ln + 1}`);

    const addr = (rec[1] << 8) | rec[2];
    const data = rec.subarray(4, 4 + len);
    switch (rec[3]) {
      case 0x00: chunks.push({ addr: base + addr, data }); break;
      case 0x01: ended = true; break;
      case 0x02: base = ((data[0] << 8) | data[1]) * 16; break;
      case 0x04: base = ((data[0] << 8) | data[1]) * 0x10000; break;
      case 0x03: case 0x05: break; // стартовый адрес — не нужен
      default: throw new FirmwareError(`Неизвестный тип записи в HEX-файле, строка ${ln + 1}`);
    }
  }
  if (!chunks.length) throw new FirmwareError('HEX-файл не содержит данных');
  return chunks;
}

// ---------- UF2 ----------

export function parseUf2(buffer) {
  const bytes = new Uint8Array(buffer);
  if (bytes.length % 512 !== 0) throw new FirmwareError('Размер UF2-файла не кратен 512 байтам');
  const v = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const chunks = [];

  for (let off = 0; off < bytes.length; off += 512) {
    if (v.getUint32(off, true) !== 0x0a324655 || v.getUint32(off + 4, true) !== 0x9e5d5157 ||
        v.getUint32(off + 508, true) !== 0x0ab16f30) {
      throw new FirmwareError('Файл не похож на UF2');
    }
    const flags = v.getUint32(off + 8, true);
    if (flags & 0x1) continue; // блок не для основной флеш-памяти
    if ((flags & 0x2000) && v.getUint32(off + 28, true) !== UF2_FAMILY_NRF52840) {
      throw new FirmwareError('UF2-файл собран не для nRF52840');
    }
    const addr = v.getUint32(off + 12, true);
    const size = v.getUint32(off + 16, true);
    if (size > 476) throw new FirmwareError('Повреждённый UF2-файл');
    chunks.push({ addr, data: bytes.subarray(off + 32, off + 32 + size) });
  }
  if (!chunks.length) throw new FirmwareError('UF2-файл не содержит данных');
  return chunks;
}

// Склеивает фрагменты в непрерывный образ так же, как nrfutil (nrfhex.py):
// UICR отбрасывается, пропуски заполняются 0xFF, размер выравнивается до 4 байт.
export function chunksToImage(chunks) {
  const flash = chunks.filter((c) => c.addr < UICR_START && c.data.length);
  if (!flash.length) throw new FirmwareError('В файле нет данных для флеш-памяти');

  let start = Infinity;
  let end = 0;
  for (const c of flash) {
    start = Math.min(start, c.addr);
    end = Math.max(end, c.addr + c.data.length);
  }
  start = Math.max(start, MBR_END);
  if (end - start > APP_MAX_SIZE) {
    throw new FirmwareError(`Прошивка занимает ${end - start} байт, а для приложения доступно ${APP_MAX_SIZE}`);
  }

  const bin = new Uint8Array((end - start + 3) & ~3).fill(0xff);
  for (const c of flash) {
    const from = Math.max(c.addr, start);
    if (from >= c.addr + c.data.length) continue;
    bin.set(c.data.subarray(from - c.addr), from - start);
  }
  return { start, bin };
}

// ---------- Проверки ----------

// Образ приложения начинается с таблицы векторов: указатель стека в RAM
// и адрес обработчика Reset (thumb) внутри области приложения.
function checkVectorTable(bin) {
  if (bin.length < 8) return false;
  const v = new DataView(bin.buffer, bin.byteOffset, bin.byteLength);
  const sp = v.getUint32(0, true);
  const reset = v.getUint32(4, true);
  return (sp >>> 20) === 0x200 && (reset & 1) === 1 &&
    reset > APP_START && reset < APP_START + APP_MAX_SIZE;
}

function validate({ bin, dat }) {
  if (!bin.length) throw new FirmwareError('Пустая прошивка');
  if (bin.length > APP_MAX_SIZE) {
    throw new FirmwareError(`Прошивка занимает ${bin.length} байт, а для приложения доступно ${APP_MAX_SIZE}`);
  }
  if (dat.length < 12) throw new FirmwareError('Init-пакет в DFU-архиве слишком короткий');
  const datCrc = dat[dat.length - 2] | (dat[dat.length - 1] << 8);
  if (datCrc !== crc16(bin)) throw new FirmwareError('Контрольная сумма прошивки не совпадает — файл повреждён');
  if (!checkVectorTable(bin)) {
    throw new FirmwareError('Файл не похож на прошивку nRF52840 для SoftDevice S140 6.1.1');
  }
}

function fromChunks(chunks, format) {
  const { start, bin } = chunksToImage(chunks);
  if (start !== APP_START) {
    throw new FirmwareError(
      `Прошивка начинается с адреса 0x${start.toString(16)}, а должна с 0x${APP_START.toString(16)} ` +
      '(приложение для SoftDevice S140 6.1.1)');
  }
  return { bin, dat: makeInitPacket(bin), format };
}

function detectFormat(name, bytes) {
  const ext = name.toLowerCase().split('.').pop();
  if (bytes[0] === 0x50 && bytes[1] === 0x4b) return 'zip';
  if (bytes.length >= 4 && new DataView(bytes.buffer, bytes.byteOffset).getUint32(0, true) === 0x0a324655) return 'uf2';
  if (bytes[0] === 0x3a) return 'hex';
  if (['zip', 'hex', 'uf2'].includes(ext)) return ext;
  return null;
}

// Возвращает { bin, dat, format } или бросает FirmwareError с понятным описанием
export async function loadFirmware(name, buffer) {
  const bytes = new Uint8Array(buffer);
  let fw;
  try {
    switch (detectFormat(name, bytes)) {
      case 'zip': fw = { ...(await parseDfuZip(buffer)), format: 'zip' }; break;
      case 'uf2': fw = fromChunks(parseUf2(buffer), 'uf2'); break;
      case 'hex': fw = fromChunks(parseIntelHex(new TextDecoder().decode(bytes)), 'hex'); break;
      default: throw new FirmwareError('Поддерживаются файлы .zip (DFU-пакет), .hex и .uf2');
    }
  } catch (e) {
    if (e instanceof FirmwareError) throw e;
    // обрезанный архив, битые сжатые данные и т. п.
    throw new FirmwareError(`Файл повреждён или имеет неизвестный формат (${e.message})`);
  }
  validate(fw);
  return fw;
}
